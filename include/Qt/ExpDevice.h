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

#pragma once

#include "SaveobComp.h"
#include "SaveobToXml.h"
#include "BString.h"
#include <QObject>
#include "StdMap.h"
#include "QtUtils.h"

class ExpWidget;		//forward declaration

//Base class for all hardware devices
class ExpDevice : public QObject
{
	Q_OBJECT

public:
	ExpDevice(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		QObject(parent),
		devData("ExpDevice"),
		saveData("ExpDevice"),
		devNode(theDevNode),
		saveNode(theSaveNode),
		widget(nullptr)
		{
		//Not in saveob
		fSerialDevice = false;
		fWidgetable = false;

		//Take the name of the node and make it the name of the saveob
		BString devNodeName = devNode.name();
		devNodeName.Trim();
		devData.SetObName(devNodeName);		//both savable object inherit the name from the configuration node
		saveData.SetObName(devNodeName);

		//Saveob
		devData.AddChildAndOwn("type", devType);
		devData.AddChildAndOwn("show", fShowWidget);
		devData.AddChildAndOwn("screen", screenName);
		devData.AddChildAndOwn("screenX", screenX);
		devData.AddChildAndOwn("screenY", screenY);
		devData.AddChildAndOwn("showBox", fShowBox);
		devData.AddChildAndOwn("boxText", boxText);

		//Default values for saveob data
		fShowWidget = false;
		fShowBox = false;
		screenX = 0;
		screenY = 0;
		fShowWidget = false;
		fShowBox = false;
	}
	virtual ~ExpDevice(){}

//Pure virtual functions to be redefined in all devices
	virtual void Dependencies(CHArray<BString>& outList) = 0;				//device adds dependency names to the array
	virtual bool Initialize(StdMap<BString,ExpDevice*>& devMap) = 0;		//device connects to other devices
	virtual void PostInitialize() = 0;										//whatever needs to be done after everything is connected
///////////////////////////////////////////////////////

public:
	BString Name() const { return devData.ObName(); }
	BString Type() const { return devType; }
	bool IsShown() const { return fShowWidget; }
	bool IsBoxShown() const { return fShowBox; }
	BString BoxText() const { return boxText; }
	bool IsWidgetable() const { return fWidgetable; }
	BString Screen() const { return screenName; }
	int ScreenX() const { return screenX; };
	int ScreenY() const { return screenY; }
	xml_node& DevNode() { return devNode; }
	xml_node& SaveNode() { return saveNode; }

	void SetName(const BString& newName) { devData.SetObName(newName); }
	void SetType(const BString& newType) { devType = newType; }
	void SetWidget(ExpWidget* theWidget) { widget = theWidget; }

	void UpdateWidget();	//If there's a widget, will call widget's FromDevice() method
	void UpdateDevice();	//If there's a widget, will call widget's ToDevice() method

	//Every device should call this function after creation to read its saved data
	//Reads both devData and saveData
	void Load()
	{
		SaveobToXml::ReadSaveobDataFromNode(devData, devNode);
		SaveobToXml::ReadSaveobDataFromNode(saveData, saveNode);
	}

	void WriteSaveDataToNode();			//Save all savable data to saveNode

	void EmitError(const BString& error)
	{
		emit SignalError("Device " + Name() + ": " + error);
	}

signals:
	void SignalError(BString error);

public slots:
	virtual void OnClose();			//Will be called by the device manager before closing

protected:
	//Device configuration saveob
	SaveobComp devData;
	//Saved data saveob
	SaveobComp saveData;

protected:
	//Not in saveob
	bool fSerialDevice;		//Whether it's a device connected by a serial port
	bool fWidgetable;		//Whether a widget can be attached to it

private:
	BString devType;
	bool fShowWidget;
	BString screenName;
	int screenX;
	int screenY;
	bool fShowBox;
	BString boxText;
	
	//Pointer to the widget
	ExpWidget* widget;

	//Two xml nodes: device node (configuration, read-only) and save node (data saving, read-write)
	xml_node devNode;
	xml_node saveNode;
};