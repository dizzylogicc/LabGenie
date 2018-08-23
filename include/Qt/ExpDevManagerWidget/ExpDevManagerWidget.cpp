/* Copyright (c) 2018 Peter Kondratyuk. All Rights Reserved.
*
* You may use, distribute and modify the code in this file under the terms of the MIT License, however
* if this file is included as part of a larger project, the project as a whole may be distributed under a different
* license.
*
* MIT license:
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
* documentation files (the "Software"), to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
* to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions
* of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#include "ExpDevManagerWidget.h"
#include "SaveobToXml.h"
#include <QLayout>
#include <QFile>

//Devices and widgets
#include "Qt/AnalogWriter/HwPhidgets1002.h"
#include "Qt/AnalogWriter/WriterPhidgets1002.h"
#include "Qt/TempController/TempController.h"
#include "Qt/TempController/TempControllerWidget.h"
#include "Qt/Qms/ExpDeviceQmsHidenHAL.h"
#include "Qt/Qms/QmsWidget.h"
#include "Qt/Tpd/ExpDeviceTpd.h"
#include "Qt/Tpd/TpdWidget.h"
#include "Qt/AnalogReader/HwPhidgets1048.h"
#include "Qt/AnalogReader/ReaderWidget.h"

//NI devices
#ifdef WITH_NI_HARDWARE
	#include "Qt/AnalogReader/NiDaqAnalogReader.h"
	#include "Qt/AnalogWriter/NiDaqAnalogWriter.h"
	#include "Qt/AnalogReader/HwNI9211.h"
	#include "Qt/AnalogReader/TempReaderNI9211.h"
#endif //WITH_NI_HARDWARE

Q_DECLARE_METATYPE(BString)

ExpDevManagerWidget::ExpDevManagerWidget(QWidget* parent) :
QTabWidget(parent),
configFileName("config.xml"),
saveFileName("save.xml")
{
	qRegisterMetaType<BString>();

	//Create an info widget
	infoWidget = new DevInfoWidget(this);

	//Read the save file
	//If read faiure or incompatible structure, recreate the correct structure
	if (!SimplestXml::ReadNodeFromFile(saveFileName, saveDoc)) SimplestXml::RemoveAllChildren(saveDoc);
	xml_node mainSaveNode = saveDoc.child("save");
	if (!mainSaveNode) mainSaveNode = saveDoc.append_child("save");
	
	//Read the configuration file
	if (!QFileInfo::exists(configFileName.c_str()))
	{
		ShowManagerError("Cannot find configuration file " + configFileName +
			". Unable to proceed.");
		return;
	}

	if (!SimplestXml::ReadNodeFromFile(configFileName, configDoc))
	{
		ShowManagerError("Error parsing XML in configuration file " + configFileName + 
						". Unable to proceed.");
		return;
	}

	//Config xml structure:
	//-config
	//--devices
	//---dev1
	//---dev2
	//--screens
	//---screen1
	//---screen2

	xml_node configNode = configDoc.child("config");
	if (!configNode)
	{
		ShowManagerError("Missing root <config> node in configuration file " + configFileName +
						". Unable to proceed.");
		return;
	}

	//Get the devices node
	xml_node devicesNode = configNode.child("devices");
	if (!devicesNode)
	{
		ShowManagerError("Missing <devices> node in configuration file " + configFileName +
			". Unable to proceed.");
		return;
	}

	//Iterate over all children of the <devices> node
	//Each child is a device
	//Create the devices and place them into deviceMap
	for(xml_node curDevNode = devicesNode.first_child();
		curDevNode;
		curDevNode=curDevNode.next_sibling())
	{
		if (curDevNode.type() != node_element) continue;

		CreateDeviceFromNode(curDevNode, mainSaveNode);
	}

	//Connect all devices in deviceMap to the error slot
	for (auto& dev : deviceMap)
	{
		QObject::connect(dev.second, &ExpDevice::SignalError, this, &ExpDevManagerWidget::OnDeviceError);
	}

	//Initialize devices recursively
	//Will use deviceMap to access all devices
	//Devices that fail to initialize will be removed from deviceMap
	InitializeDevices();

	//Call PostInitialize() on all the devices that are left in the deviceMap
	//and add them to the device list with their dependencies
	for (auto& elem : deviceMap)
	{
		BString curName = elem.first;
		ExpDevice* curDev = elem.second;

		curDev->PostInitialize();

		BString infoString = curName;
		CHArray<BString> dependencies;
		curDev->Dependencies(dependencies);

		if (!dependencies.IsEmpty())
		{
			infoString += " (depends on: ";

			for (auto& dep : dependencies) infoString += (dep + ", ");
		
			infoString = infoString.Left(infoString.GetLength() - 2);
			infoString += ")";
		}

		ShowDeviceInList(infoString);
	}
	
	//Create all the screens
	xml_node screensNode = configNode.child("screens");
	if (!screensNode)
	{
		ShowManagerError("Missing <screens> node in configuration file " + configFileName +
			". No widgets will be shown.");

		return;
	}

	for (xml_node curScreenNode = screensNode.first_child();
		curScreenNode;
		curScreenNode = curScreenNode.next_sibling())
	{
		if (curScreenNode.type() != node_element) continue;

		CreateScreenFromNode(curScreenNode);
	}

	//Iterate through the devices and place device widgets on screens where needed
	for (auto& cur : deviceMap)
	{
		ExpDevice& dev = *cur.second;

		if (dev.IsWidgetable() && dev.IsShown())
		{
			//Yes, the user wants to show this device as a widget
			BString screen = dev.Screen();
			
			if (screen == "")
			{
				ShowManagerError("Device " + dev.Name() + " requests to be shown, but no <screen> "
									"is specified. Device will not be shown.");
				continue;
			}

			if (!screenMap.IsPresent(screen))
			{
				ShowManagerError("Device " + dev.Name() + " requests to be shown on screen " +
							screen + ", but such screen does not exist. Device will not be shown.");
				continue;
			}

			QGridLayout* gridLayout = screenMap[screen];

			//create the widget of the appropriate type
			//by attempting dynamic casts
			ExpWidget* widget;
			if (auto p = dynamic_cast<TempController*>(&dev)) widget = new TempControllerWidget(p, this);
			else if (auto p = dynamic_cast<ExpDeviceQmsHidenHAL*>(&dev)) widget = new QmsWidget(p, this);
			else if (auto p = dynamic_cast<ExpDeviceTpd*>(&dev)) widget = new TpdWidget(p, this);

			//Classes higher in the hierarchy must be tried after their derived classes are tried
			else if (auto p = dynamic_cast<AnalogReader*>(&dev)) widget = new ReaderWidget(p, this);

			else
			{
				ShowManagerError("Cannot show device " + dev.Name() + " as a widget: no appropriate widget found.");
				continue;
			}

			//If a box is shown
			if (dev.IsBoxShown())
			{
				BString text = dev.BoxText();
				if (text == "") text = dev.Name();

				QGroupBox* groupBox = new QGroupBox(this);
				gridLayout->addWidget(groupBox, dev.ScreenY(), dev.ScreenX());
				groupBox->setTitle(text.c_str());

				QVBoxLayout* vLayout = new QVBoxLayout(groupBox);
				groupBox->setLayout(vLayout);

				vLayout->addWidget(widget);
			}
			//no box
			else gridLayout->addWidget(widget, dev.ScreenY(), dev.ScreenX());
		}
	}


	//Finally create the Device Manager tab
	addTab(infoWidget, "Device manager");
	infoWidget->setAutoFillBackground(true);
}

void ExpDevManagerWidget::InitializeDevices()
{
	//Two empty maps
	StdMap<BString> initMap;		//Devices that were successfully initialized
	StdMap<BString> failedMap;		//Devices that failed

	//Call recursive initialization on each device in our map
	for (auto& elem : deviceMap) RecursiveInitialize(elem.first, initMap, failedMap);

	//Remove the devices that failed to initialize from the deviceMap
	for (auto& elem : failedMap)
	{
		delete deviceMap[elem.first];
		deviceMap.Remove(elem.first);
	}
}

void ExpDevManagerWidget::RecursiveInitialize(const BString& devName,
	StdMap<BString>& initMap, StdMap<BString>& failedMap)
{
	//If the name is empty, or if this device does not exist, or is already successfully initialized, or failed, just return
	if (devName == "" || initMap.IsPresent(devName) || failedMap.IsPresent(devName) || !deviceMap.IsPresent(devName)) return;

	//This is a new device, it exists, and we haven't yet tried to initialize it
	ExpDevice* curDevice = deviceMap[devName];

	//What are the dependencies?
	CHArray<BString> depArray;
	curDevice->Dependencies(depArray);

	//Call RecursiveInitialize() on each dependency
	for (auto& dep : depArray) RecursiveInitialize(dep, initMap, failedMap);

	//Have we failed yet?
	bool fFailed = false;

	//Check every dependence to make sure that they exist and have initialized successfully
	for (auto& dep : depArray)
	{
		if (dep == "")
		{
			ShowManagerError("Device " + devName + " is missing one of the required dependencies. "
				"Cannot create " + devName + ".");
			fFailed = true;
			continue;
		}

		else if (!deviceMap.IsPresent(dep))
		{
			ShowManagerError("Device " + devName + " depends on device " + dep +
				", but " + dep + " does not exist. Cannot create " + devName + ".");
			fFailed = true;
			continue;
		}

		else if (failedMap.IsPresent(dep))
		{
			ShowManagerError("Device " + devName + " depends on device " + dep +
				", but " + dep + " could not be created. Cannot create " + devName + ".");
			fFailed = true;
			continue;
		}
	}

	//If any of the dependencies are bad, fail this device too
	if (fFailed)
	{
		failedMap.Insert(devName);
		return;
	}

	//Let's try to initialize it
	if (curDevice->Initialize(deviceMap)) initMap.Insert(devName);	//success
	else															//failure
	{
		failedMap.Insert(devName);
		ShowManagerError("Device " + devName + " could not be created because it "
							"failed to initialize.");
	}

	return;
}

void ExpDevManagerWidget::OnClose()
{
	//Call OnClose() on all devices
	for (auto& cur : deviceMap) cur.second->OnClose();

	//Save the XML save file
	SimplestXml::WriteNodeToFile(saveFileName, saveDoc, true, true);
}

void ExpDevManagerWidget::CreateDeviceFromNode(xml_node& node, xml_node& mainSaveNode)
{
	BString devName = node.name();
	devName.Trim();

	if (devName == "")
	{
		ShowManagerError("Device with an empty name found. Device will not be created.");
		return;
	}

	if (deviceMap.IsPresent(devName))
	{
		ShowManagerError("Device with the name " + devName + " already exists. Another device"
						" with the same name cannot be created.");
		return;
	}

	xml_node typeNode = node.child("type");

	if (!typeNode)
	{
		ShowManagerError("Missing <type> node for device " + BString(node.name()) +
			". Device will not be created.");
		return;
	}

	BString type = typeNode.first_child().value();
	type.Trim();

	if (type == "")
	{
		ShowManagerError("Cannot deduce type for device " + BString(node.name()) +
			". Device will not be created.");
		return;
	}

	//Find or create the save node for the device in save XML document
	xml_node curSaveNode = mainSaveNode.child(devName);
	if (!curSaveNode) curSaveNode = mainSaveNode.append_child(devName);

	//Supported device types:

	/*
	HwNI9211
	TempReaderNI9211
	HwPhidgets1002
	WriterPhidgets1002
	TempController
	QmsHidenHAL
	Tpd
	*/

	ExpDevice* curDevice = nullptr;
		
	if (type == "HwPhidgets1002")		curDevice = new HwPhidgets1002(node, curSaveNode, this);
	else if (type == "HwPhidgets1048")		curDevice = new HwPhidgets1048(node, curSaveNode, this);
	else if (type == "WriterPhidgets1002")	curDevice = new WriterPhidgets1002(node, curSaveNode, this);
	else if (type == "TempController")		curDevice = new TempController(node, curSaveNode, this);
	else if (type == "QmsHidenHAL")			curDevice = new ExpDeviceQmsHidenHAL(node, curSaveNode, this);
	else if (type == "Tpd")					curDevice = new ExpDeviceTpd(node, curSaveNode, this);
	else if (type == "TempReader1048")		curDevice = new TempReader1048(node, curSaveNode, this);

	//NI devices
#ifdef WITH_NI_HARDWARE
	else if (type == "NiDaqAnalogReader")	curDevice = new NiDaqAnalogReader(node, curSaveNode, this);
	else if (type == "NiDaqAnalogWriter")	curDevice = new NiDaqAnalogWriter(node, curSaveNode, this);
	else if (type == "HwNI9211")			curDevice = new HwNI9211(node, curSaveNode, this);
	else if (type == "TempReaderNI9211")	curDevice = new TempReaderNI9211(node, curSaveNode, this);
#endif //WITH_NI_HARDWARE

	//Type not found
	else
	{
		ShowManagerError("Unknown type " + type + " for device " + BString(node.name()) +
			". Device will not be created.");
		mainSaveNode.remove_child(devName);
		return;
	}

	//Add the device to map (enumerated by names)
	deviceMap[devName] = curDevice;
}

void ExpDevManagerWidget::CreateScreenFromNode(xml_node& node)
{
	BString name = node.name();
	BString label = node.child("label").first_child().value();

	QWidget* widget = new QWidget;
	addTab(widget, label.c_str());
	//widget->setStyleSheet(tabStyleSheet);
	widget->setAutoFillBackground(true);
	

	QGridLayout* layout = new QGridLayout(widget);
	widget->setLayout(layout);

	screenMap[name] = layout;
}