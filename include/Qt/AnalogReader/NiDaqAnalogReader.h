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
#include "NIDAQmx.h"
#include <atomic>
#include <mutex>
#include <thread>

class NiDaqAnalogReader : public AnalogReader
{
	Q_OBJECT

public:
	NiDaqAnalogReader(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
	AnalogReader(theDevNode, theSaveNode, parent)
	{
		fContinuousOn = false;
		fThreadRunning = false;
		taskHandle = 0;

		//Default values
		NIdeviceName = "Dev1";
		mode = "Diff";
		samplingRate = 20000;
		averagingSamples = 100;
		minVolt = -10;
		maxVolt = 10;
		period = 0.1;

		//Saveob
		devData.AddChildAndOwn("NIdeviceName", NIdeviceName);
		devData.AddChildAndOwn("channel", channel);
		devData.AddChildAndOwn("mode", mode);
		devData.AddChildAndOwn("samplingRate", samplingRate);
		devData.AddChildAndOwn("averagingSamples", averagingSamples);
		devData.AddChildAndOwn("minVolt", minVolt);
		devData.AddChildAndOwn("maxVolt", maxVolt);
		devData.AddChildAndOwn("period", period);

		//Load all data
		Load();

		//Make sure that the period is realistic
		if (period < averagingSamples / samplingRate)
		{
			period = averagingSamples / samplingRate;

			EmitError("Specified period is too small for specified averagingSamples and samplingRate."
				"Period will be set at averagingSamples / samplingRate;");
		}
	}

	~NiDaqAnalogReader(){ if (taskHandle != 0) DAQmxClearTask(taskHandle); }

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){}
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap);
	virtual void PostInitialize(){}

public:
	double InternalReadOnce();		//Does not emit new data signal
	
	void StartContinuous()
	{
		if (fContinuousOn) return;

		//The acquisition thread should not be running, but if it is, wait for it to shut down
		while (fThreadRunning) std::this_thread::sleep_for(std::chrono::milliseconds(20));

		fContinuousOn = true;

		//Start the acquisition thread
		std::thread t(&NiDaqAnalogReader::AcquisitionThread, this);
		t.detach();
	}
	
	void StopContinuous(){ fContinuousOn = false; }
	
	double Period() { return period; }

private:
	void AcquisitionThread();

public:
	//In Saveob
	BString NIdeviceName;			//The name of the hardware device that the reader will attach to
	int channel;					//The channel that this reader connects to; range depends on particular DAQ board (e.g., ai0 to ai7)
	BString mode;					//"Diff"-differential, "PseudoDiff", "RSE"-referenced single-ended, "NRSE" - non-referenced
	double samplingRate;			//Sampling rate in samples per second
	int averagingSamples;			//Number of samples to collect for averaging
	double minVolt;					//Minimum and maximum voltage expected
	double maxVolt;					
	double period;					//How frequently the measurement is triggered

private:
	std::atomic<bool> fContinuousOn;
	std::atomic<bool> fThreadRunning;
	CHArray<double> buffer;
	TaskHandle taskHandle;
	std::recursive_mutex mutex;		//protects the ReadOnce() function
};

#endif //WITH_NI_HARDWARE