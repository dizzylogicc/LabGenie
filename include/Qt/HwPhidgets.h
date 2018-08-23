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

#include "Qt/ExpDevice.h"
#include "phidget21.h"
#include <mutex>

//Base hardware class for Phidgets devices

class HwPhidgets : public ExpDevice
{
	Q_OBJECT

public:
	HwPhidgets(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		ExpDevice(theDevNode, theSaveNode, parent)
	{
		devData.AddChildAndOwn("serial", serial);

		attachPeriodMs = 3000;
	}

	virtual bool Start()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		if (!baseHandle) return false;

		CPhidget_open(baseHandle, serial);	//open the Analog for device connections

		//get the program to wait for an Analog device to be attached
		//Returns zero on error
		if (CPhidget_waitForAttachment(baseHandle, attachPeriodMs))
		{
			BString str;
			str.Format("%i", serial);
			EmitError("Could not start Phidgets device with serial# " + str + ".");

			baseHandle = 0;
			return false;
		}

		return true;
	}

	virtual bool Stop()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		if (baseHandle == 0) return false;

		//Close the device
		CPhidget_close(baseHandle);
		CPhidget_delete(baseHandle);
		return true;
	}

	~HwPhidgets(){}


public:
	//Saveob
	int serial;				//Serial number of the Phidgets device

protected:
	int attachPeriodMs;					//Number of milliseconds to wait for device attachment
	//Base handle for operations common to all devices
	//Needs to be set in the derived class before it can be used
	CPhidgetHandle baseHandle;
	std::recursive_mutex mutex;			//Mutex that protects ALL private data
};