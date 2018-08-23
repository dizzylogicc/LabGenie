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

#include "AnalogWriter.h"
#include "HwPhidgets1002.h"

//Analog voltage writer that writes to the HwPhidgets1002 4-channel voltage output

class WriterPhidgets1002 : public AnalogWriter
{
	Q_OBJECT

public:
	WriterPhidgets1002(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		AnalogWriter(theDevNode, theSaveNode, parent)
	{
		hwPhidgets = nullptr;

		devData.AddChildAndOwn("channel", channel);
		devData.AddChildAndOwn("hardware", hardware);

		//Load all data
		Load();
	}

	~WriterPhidgets1002(){}

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){ outList.AddAndExtend(hardware); }
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap)
	{
		//Try to cast the hardware name to HwNI9211 and return false is failed
		HwPhidgets1002* dev = dynamic_cast<HwPhidgets1002*>(devMap[hardware]);
		if (!dev) return false;

		//Everything's OK - add the reader to the hardware
		SetHardware(dev);
		return true;
	}
	virtual void PostInitialize(){}

public:
	void WriteOnce(double voltage)
	{
		if (hwPhidgets) hwPhidgets->Write(voltage, channel);
	}

	void SetHardware(HwPhidgets1002* hw) { hwPhidgets = hw; }

public:
	//In saveob
	int channel;					//channel number on the hardware device that this writer is associated with
	BString hardware;				//The name assigned to the Phidgets1002 hardware

private:
	HwPhidgets1002* hwPhidgets;
};