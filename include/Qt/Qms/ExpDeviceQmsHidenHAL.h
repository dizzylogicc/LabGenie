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

#include "ExpDeviceQms.h"
#include "serial/serial.h"
#include <queue>

//Device class for Hiden HAL QMS
class ExpDeviceQmsHidenHAL : public ExpDeviceQms
{
	Q_OBJECT

public:
	ExpDeviceQmsHidenHAL(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		ExpDeviceQms(theDevNode, theSaveNode, parent)
	{
		fSerialDevice = true;

		termToQms = "\x0D";					//terminator is 0D for the messages going to QMS
		termFromQms = "\x0D\x0A";			//And 0D 0A going in the other direction

		//Load all data
		Load();
	}
	virtual ~ExpDeviceQmsHidenHAL();

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){}
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap){ return true; }
	virtual void PostInitialize(){}

public:
	void Start();		//Starts scanning
	void PowerOn();
	void PowerOff();

public:
	virtual bool Connect();
	virtual BString GetIdString() { return SendReceive("pget name"); }			//Get the ID string from the mass spec

private:
	void AcquisitionThread();		//The thread that collects the data from the mass spec
	void SetSpecParams();			//The function that sets the parameters specific to mass spectrum acquisition
	void SetSigTimeParams();		//Set parameters for signal vs. time acquisition
	void ProcessDataString(BString& str);
	void ShutOffDataAndAbort();		//Shuts off data and sets the state to Abort: independent of current state
	void ShutOffPower() { SendReceive("lset mode 0"); }		//Tries to shut off power independent of state
};