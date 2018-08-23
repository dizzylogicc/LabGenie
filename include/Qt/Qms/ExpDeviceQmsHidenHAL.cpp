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

#include "ExpDeviceQmsHidenHAL.h"

#include "QtUtils.h"

bool ExpDeviceQmsHidenHAL::Connect()
{
	Disconnect();

	//Create new serial connection to the mass spec
	port.reset(
		new serial::Serial(
			portString,
			115200,
			serial::Timeout(500, 500, 100, 500, 100),
			serial::eightbits,
			serial::parity_none,
			serial::stopbits_one,
			serial::flowcontrol_none
							)
				);

	//Flush the port
	port->flush();

	if (!port->isOpen()) return false;

	//Here we are sure we can communicate with the mass spec

	ShutOffDataAndAbort();		//If somehow we are scanning, stop
	ShutOffPower();				//If for any reason the power is on, shut it off
	
	SetConnState(connState_on);
	return true;
}

ExpDeviceQmsHidenHAL::~ExpDeviceQmsHidenHAL()
{
	//Before the object is destroyed, make sure the QMS is shut off
	if (port && port->isOpen())
	{
		ShutOffDataAndAbort();
		ShutOffPower();
	}
}

void ExpDeviceQmsHidenHAL::Start()
{
	if (ScanState() != scanState_standby) return;		//Cannot start acquisition unless the mass spec is in the standby mode

	if (opMode == opMode_sigTime && massTable.Count() == 0)
	{
		QtUtils::ErrorBox("Unable to start acquisition: no masses are specified in the mass table.");
		return;
	}

	SetScanState(scanState_scanning);

	//Start the acquisition thread
	std::thread acqThread(&ExpDeviceQmsHidenHAL::AcquisitionThread, this);
	acqThread.detach();
}

//Turn the multiplier and the emission on
void ExpDeviceQmsHidenHAL::PowerOn()
{
	//If we are scanning or already on, do nothing!
	if(ScanState() != scanState_standby || PowerState() != powerState_off) return;

	//LPUT (device) followed by up to 7 parameters sets the device state in up to 7 modes (from 0 up)
	
	//Set the states of the two filaments
	SendReceive("lput F1 0 %i", (filament == 1) ? 1 : 0);
	SendReceive("lput F2 0 %i", (filament == 2) ? 1 : 0);
		
	SendReceive("lput multiplier 0 %i", int(semVoltage));		//Set the multiplier voltage
	SendReceive("lput emission 20 %i", int(emission));			//Set emission current
	SendReceive("lset mode 1");									//Set the mode to 1 (on)

	SetPowerState(powerState_on);
}

//Turn the multiplier and the emission off
void ExpDeviceQmsHidenHAL::PowerOff()
{
	if (ScanState() != scanState_standby) return;	//If we are scanning, do nothing!
	ShutOffPower();
	SetPowerState(powerState_off);					//Set the new power state
}

void ExpDeviceQmsHidenHAL::AcquisitionThread()
{
	emit SignalDatasetStart();		//A single spectrum, multiple spectrum or signal-time dataset is starting
	timer.SetTimerZero(0);

	if (opMode == opMode_spectrum) SetSpecParams();
	else if (opMode == opMode_sigTime) SetSigTimeParams();

	SendReceive("pset terse 1");
	SendReceive("pset points 100");
	SendReceive("data on");
	SendReceive("lini Ascans");
	SendReceive("sjob lget Ascans");

	//Read the data
	BString cur = SendReceive("data all");
	bool fInsideBrackets = false;	//Whether we are inside square brackets, [ ... ]
									//Data comes in runs within square brackets, 
									//(either spectra or sets of multi-mass measurements)
	bool fDataEnded = false;		//Whether the data stream has ended - '!' termination received

	BString dataString;
	while(1)
	{
		for (int i = 0; i < cur.length(); i++)
		{
			char symbol = cur[i];
			if (symbol == '!') fDataEnded = true;

			if (fInsideBrackets)	//We are already inside brackets
			{
				if (symbol == ']')
				{
					fInsideBrackets = false;
					ProcessDataString(dataString);
					if (opMode == opMode_spectrum) emit SignalSpecEnd();			//Emit spec start signal if in spectrum mode
					else if (opMode == opMode_sigTime) emit SignalMIDend();
				}
				else dataString += symbol;
			}
			else if (symbol == '[')
			{
				fInsideBrackets = true;
				if(opMode == opMode_spectrum) emit SignalSpecStart();			//Emit spec start signal if in spectrum mode
				else if (opMode == opMode_sigTime) emit SignalMIDstart();
			}
		}

		ProcessDataString(dataString);

		if (!fDataEnded && ScanState() != scanState_stopNow)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			cur = SendReceive("data");
		}
		else break;
	}

	ShutOffDataAndAbort();

	emit SignalDatasetEnd();
	SetScanState(scanState_standby);
}

void ExpDeviceQmsHidenHAL::ShutOffDataAndAbort()
{
	SendReceive("data stop");
	SendReceive("data off");

	BString stateResp;

	do
	{
		SendReceive("sset state Abort:");
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		stateResp = SendReceive("sget state");
	}
	while (stateResp.Find("Run:") != -1);		//wait for the Run: state to end
}

void ExpDeviceQmsHidenHAL::SetSpecParams()
{
	SendReceive("sdel all");
	SendReceive("sset scan Ascans");
	SendReceive("sset row 1");
	SendReceive("sset output mass");

	SendReceive("sset start %.2f", specStart);
	SendReceive("sset stop %.2f", specEnd);
	SendReceive("sset step %.3f", specStep);

	SendReceive("sset input %s", (detectorType == 0) ? "SEM" : "Faraday");

	SendReceive("sset low %i", (fHidenAutoranging) ? hidenMinRange : hidenCurRange);
	SendReceive("sset high %i", (fHidenAutoranging) ? hidenMaxRange : hidenCurRange);
	SendReceive("sset current %i", hidenCurRange);

	SendReceive("sset dwell %i%", hidenDwell);
	SendReceive("sset settle %i%", hidenSettleTime);

	SendReceive("sset mode 1");
	SendReceive("sset report 5");

	//Set number of cycles, 1 or 0 (continuous scanning)
	SendReceive("pset cycles %i", (specMode == specMode_single) ? 1 : 0);
}

void ExpDeviceQmsHidenHAL::SetSigTimeParams()
{
	SendReceive("sdel all");
	SendReceive("sset scan Ascans");

	for (int i = 0; i < massTable.Count(); i++)
	{
		SendReceive("sset row %i", i+1);
		SendReceive("sset output mass");

		SendReceive("sset start %.2f", massTable[i]);
		SendReceive("sset stop %.2f", massTable[i]);
		SendReceive("sset step 1");

		SendReceive("sset input %s", (detectorType == 0) ? "SEM" : "Faraday");

		SendReceive("sset low %i", (fHidenAutoranging) ? hidenMinRange : hidenCurRange);
		SendReceive("sset high %i", (fHidenAutoranging) ? hidenMaxRange : hidenCurRange);
		SendReceive("sset current %i", hidenCurRange);

		SendReceive("sset dwell %i%", hidenDwell);
		SendReceive("sset settle %i%", hidenSettleTime);

		SendReceive("sset mode 1");
		SendReceive("sset report 5");
	}
	
	//Set continuous scanning
	SendReceive("pset cycles 0");
}

void ExpDeviceQmsHidenHAL::ProcessDataString(BString& str)
{
	if (str == "") return;

	int pos = 0;
	BString delimiters = "[]{};:, !";

	CHArray<double> vals(200);
	BString token = str.Tokenize(delimiters, pos);
	while (token != "")
	{
		vals << atof(token);
		token = str.Tokenize(delimiters, pos);
	}

	CHArray<QmsDataPoint> newData(100);
	for (int i = 0; i < vals.Count(); i += 2)
	{
		QmsDataPoint point;

		point.mass = vals[i];
		point.signal = vals[i + 1];
		point.time = timer.GetCurTime(0);

		newData << point;
	}

	if (newData.Count() > 0) emit SignalNewData(newData);

	str = "";
}