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

#include "Qt/HwPhidgets.h"
#include <mutex>

//Hardware class for Phidgets 1002 4-channel analog voltage output

class HwPhidgets1002 : public HwPhidgets
{
	Q_OBJECT

public:
	HwPhidgets1002(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		HwPhidgets(theDevNode, theSaveNode, parent)
	{
		//Load all data
		Load();
	}

	~HwPhidgets1002(){ Stop(); }

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){}
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap){ return true; }
	virtual void PostInitialize(){ Start(); }

public:
	//Initializes the device
	bool Start()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		handle = 0;
		CPhidgetAnalog_create(&handle);	//create the Analog object
		baseHandle = (CPhidgetHandle)handle;
		
		if (!HwPhidgets::Start()) return false;

		//Enable all four outputs
		for (int i = 0; i<4; i++)
		{
			CPhidgetAnalog_setVoltage(handle, i, 0.0);
			CPhidgetAnalog_setEnabled(handle, i, PTRUE);
		}
		return true;
	}

	//Disables the device
	bool Stop()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		if (handle == 0) return false;

		//disable all outputs
		for (int i = 0; i<4; i++) CPhidgetAnalog_setEnabled(handle, i, PFALSE);

		return HwPhidgets::Stop();
	}

	//Write a voltage (-10 to 10 v) to specified channel (0-3)
	void Write(double voltage, int channel)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		if (handle == 0) return;

		CPhidgetAnalog_setVoltage(handle, channel, voltage);
	}

private:
	std::recursive_mutex mutex;						//Mutex that protects ALL private data
	CPhidgetAnalogHandle handle;
};