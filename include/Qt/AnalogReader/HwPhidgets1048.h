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
#include "TempReader1048.h"
#include <mutex>
#include <atomic>

//Hardware class for Phidgets 1048 4-channel temperature reader

class HwPhidgets1048 : public HwPhidgets
{
	Q_OBJECT

public:
	HwPhidgets1048(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		HwPhidgets(theDevNode, theSaveNode, parent),
		readers(4, true)
	{
		fRunning = false;
		periodMs = 200;			//200 ms wait time between readings

		//Load all data
		Load();
	}

	~HwPhidgets1048(){ Stop(); }


public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){}
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap){ return true; }
	virtual void PostInitialize(){ Start(); }

public:
	//Starts the device
	bool Start();
	bool Stop();
	bool AddReader(TempReader1048* theReader);
	double SampleRate() { return 1.0 / (double(periodMs) / 1000); }

private:
	void AcquisitionThread();		//The routine that continuously reads the temperature and updates the readers
	void ReadTemps();				

private:
	CPhidgetTemperatureSensorHandle handle;

	CHArray<CHArray<TempReader1048*>> readers;

	std::atomic<bool> fRunning;
	int periodMs;		//A reading routine reads all the channels, then waits for periodMs ms, then reads again
};