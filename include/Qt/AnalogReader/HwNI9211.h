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

#include "Qt/ExpDevice.h"
#include "NIDAQmx.h"
#include "Qt/AnalogReader/TempReaderNI9211.h"
#include <mutex>
#include "BidirectionalMap.h"

//Thermocouple type, min temp, max temp, and NI designation for configuring the TC source on 9211
struct TCparams9211
{
	TCparams9211(){}
	TCparams9211(const BString& theTCtype, double theMinTemp, double theMaxTemp, int theDesig)
	{
		tcType = theTCtype;
		minTemp = theMinTemp;
		maxTemp = theMaxTemp;
		desig = theDesig;
	}

	BString tcType;
	double minTemp;
	double maxTemp;
	int desig;
};

class HwNI9211 : public ExpDevice
{
	Q_OBJECT

public:
	HwNI9211(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0);
	~HwNI9211(){ Stop(); }

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){}
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap){ return true; }
	virtual void PostInitialize(){ Start(); }


public:
	bool AddReader(TempReaderNI9211* newReader);

	bool Start();			//Starts the scanning and data distribution, if not running already
	bool Stop();			//Stops scanning

	double SampleRate()		//14.28 / (Nchannels + 1)
	{ 
		std::lock_guard<std::recursive_mutex> lock(mutex);

		return 14.28 / ((double)NumOccupiedChannels() + 1.0);
	}

	//Count the channels where at least one reader is attached
	int NumOccupiedChannels()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		int count = 0;
		for (auto& cur : readers) if (!cur.IsEmpty()) count++;

		return count;
	}


private:
	static int32 CVICALLBACK DataReadyCallback(TaskHandle taskHandle, int32 everyNsamplesEventType,
		uInt32 nSamples, void *pObject);
	
public:
	//Saveob
	BString NIdeviceName;

private:
	TaskHandle taskHandle;
	bool fRunning;

	CHArray<CHArray<TempReaderNI9211*>> readers;	//Readers for this hardware arranged as four columns, one per physical channel
	CHArray<TCparams9211> tcParams;					//Config params for all types of thermocouples supported by NI9211
	CBidirectionalMap<BString> tcParamMap;			//Map between thermocouple types ("K", "N", etc.) and index in the tcParams
	std::recursive_mutex mutex;						//Mutex that protects ALL private data
};

#endif //WITH_NI_HARDWARE