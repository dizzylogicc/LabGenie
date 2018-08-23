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

#include <QTabWidget>
#include "BString.h"
#include "StdMap.h"
#include "Qt/ExpDevice.h"
#include "Qt/ExpDevManagerWidget/DevInfoWidget.h"

class ExpDevManagerWidget : public QTabWidget
{
	Q_OBJECT

public:
	ExpDevManagerWidget(QWidget* parent = nullptr);
	~ExpDevManagerWidget(){};

public slots:
	void OnClose();									//Called by the main window before closing
	void OnDeviceError(BString str) { ShowDeviceError(str); }

private:
	void CreateDeviceFromNode(xml_node& node, xml_node& mainSaveNode);
	void CreateScreenFromNode(xml_node& node);

	void ShowManagerError(const BString& str){ infoWidget->AddError("DevManager: " + str + "\n"); }
	void ShowDeviceError(const BString& str){ infoWidget->AddError(str + "\n"); }
	void ShowDeviceInList(const BString& str){ infoWidget->AddDevice(str + "\n"); }

private:
	void InitializeDevices();
	void RecursiveInitialize(const BString& devName, StdMap<BString>& initMap, StdMap<BString>& failedMap);

private:
	StdMap<BString, ExpDevice*> deviceMap;			//Device name to device pointer map
	StdMap<BString, QGridLayout*> screenMap;		//Screen name to grid layout on corresponding tab
	xml_document configDoc;				//Configuration document that stores all device configuration data; read-only
	BString configFileName;
	xml_document saveDoc;				//XML document where the devices save their data; write and read
	BString saveFileName;

private:
	DevInfoWidget* infoWidget;						//Tab with device information populated by the devManager
};
