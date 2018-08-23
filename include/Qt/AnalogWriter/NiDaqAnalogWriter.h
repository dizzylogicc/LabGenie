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

#ifdef WITH_NI_HARDWARE

#include "NIDAQmx.h"
#include "AnalogWriter.h"
#include <mutex>

//Hardware class for Phidgets 1002 4-channel analog voltage output

class NiDaqAnalogWriter : public AnalogWriter
{
	Q_OBJECT

public:
	NiDaqAnalogWriter(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		AnalogWriter(theDevNode, theSaveNode, parent)
	{
		taskHandle = 0;

		//Default values
		minVolt = -10;
		maxVolt = 10;

		//Saveob
		devData.AddChildAndOwn("NIdeviceName", NIdeviceName);
		devData.AddChildAndOwn("channel", channel);
		devData.AddChildAndOwn("minVolt", minVolt);
		devData.AddChildAndOwn("maxVolt", maxVolt);

		//Load all data
		Load();
	}

	~NiDaqAnalogWriter(){ if (taskHandle != 0) DAQmxClearTask(taskHandle); }

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){}
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap);
	virtual void PostInitialize(){}

public:
	//Write a voltage
	void WriteOnce(double val);

private:
	//Saveob
	BString NIdeviceName;			//The device name - for example Dev1, Dev2 etc.
	int channel;					//Output channel to use for writing voltage (e.g. ao0 or ao1)
	double minVolt;					//Minimum and maximum expected voltage
	double maxVolt;

private:
	std::recursive_mutex mutex;						//Mutex that protects the write function
	TaskHandle taskHandle;
};

#endif //WITH_NI_HARDWARE