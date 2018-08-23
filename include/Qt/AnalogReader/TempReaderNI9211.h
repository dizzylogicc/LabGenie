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

#include "AnalogReader.h"
#include <atomic>
#include <thread>
#include <chrono>

class HwNI9211;

class TempReaderNI9211 : public AnalogReader
{
	Q_OBJECT

public:
	TempReaderNI9211(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
	AnalogReader(theDevNode, theSaveNode, parent)
	{
		lastReading = 0;
		fFirstPointRead = false;
		fContinuousOn = false;

		//Saveob data
		hardware = "";
		tcType = "K";
		channel = 0;
		

		devData.AddChildAndOwn("hardware", hardware);
		devData.AddChildAndOwn("channel", channel);
		devData.AddChildAndOwn("tcType", tcType);

		//Load all data
		Load();
	}
	~TempReaderNI9211(){}

public:
//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList)
	{
		outList.AddAndExtend(hardware);
	}

	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap);
	virtual void PostInitialize(){}

public:
	double InternalReadOnce()		//Does not emit new data signal
	{
		while (!fFirstPointRead) std::this_thread::sleep_for(std::chrono::milliseconds(50));

		return lastReading;
	}

	void StartContinuous(){fContinuousOn = true;}

	void StopContinuous(){fContinuousOn = false;}

	void UpdateReading(double newReading)
	{
		fFirstPointRead = true;
		lastReading = newReading;
		if (fContinuousOn) EmitNewData(newReading);
	}

	void SetHardware(HwNI9211* hw)
	{
		hwNI9211 = hw;
	}

	double Period();

public:
	//In Saveob
	BString hardware;				//The name of the hardware device that the reader will attach to
	int channel;					//The thermocouple channel that this reader connects to, range is 0-3
	BString tcType;					//Letter designation of the thermocouple type, e.g. "K" or "C"

private:
	std::atomic<double> lastReading;
	std::atomic<bool> fFirstPointRead;			//Whether we have read the first point and it's safe to return value read
	std::atomic<bool> fContinuousOn;

private:
	HwNI9211* hwNI9211;
};

#endif //WITH_NI_HARDWARE