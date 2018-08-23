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
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include "QmsDataPoint.h"
#include "ExpDeviceQmsDefines.h"
#include "Timer.h"
#include "serial/serial.h"

//Base class for all QMS devices
class ExpDeviceQms : public ExpDevice
{
	Q_OBJECT

public:
	ExpDeviceQms(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		ExpDevice(theDevNode, theSaveNode, parent)
	{
		//Not in saveob
		fWidgetable = true;

		//Non-savable data initialization
		scanState = scanState_standby;			//standby initial state
		powerState = powerState_off;			//off initial state
		connState = connState_off;				//Serial is disconnected initially

		//Initialization - in saveob
		opMode = opMode_spectrum;
		detectorType = detector_SEM;

		semVoltage = 800;
		emission = 200;
		filament = 1;

		specStart = 12;
		specEnd = 50;
		specStep = 0.1;

		specMode = specMode_single;
		
		fHidenAutoranging = true;
		hidenCurRange = -7;
		hidenMaxRange = -5;
		hidenMinRange = -10;
		hidenDwell = 100;
		hidenSettleTime = 100;

		portString = "COM1";

		//Handle saveob saving and loading
		saveData.AddChildAndOwn("opMode", opMode);
		saveData.AddChildAndOwn("detectorType", detectorType);

		saveData.AddChildAndOwn("semVoltage", semVoltage);
		saveData.AddChildAndOwn("emission", emission);
		saveData.AddChildAndOwn("filament", filament);

		saveData.AddChildAndOwn("specStart", specStart);
		saveData.AddChildAndOwn("specEnd", specEnd);
		saveData.AddChildAndOwn("specStep", specStep);
		saveData.AddChildAndOwn("specMode", specMode);
		
		saveData.AddChildAndOwn("fHidenAutoranging", fHidenAutoranging);
		saveData.AddChildAndOwn("hidenCurRange", hidenCurRange);
		saveData.AddChildAndOwn("hidenMaxRange", hidenMaxRange);
		saveData.AddChildAndOwn("hidenMinRange", hidenMinRange);
		saveData.AddChildAndOwn("hidenDwell", hidenDwell);
		saveData.AddChildAndOwn("hidenSettleTime", hidenSettleTime);

		saveData.AddChildAndOwn("massTable", massTable);

		saveData.AddChildAndOwn("portString", portString);
		
	}
	virtual ~ExpDeviceQms(){}

signals:
	void SignalNewData(CHArray<QmsDataPoint> newPoints);		//QMS has received new data
	void SignalNewCom(BString str);								//QMS is sending / receiving data from the mass spec
	void SignalNewScanState(int theScanState);					//0 - standby, 1 - data acquisition, 2 - awaiting stop
	void SignalNewPowerState(int thePowerState);				//0 - MS off, 1 - MS on
	void SignalNewConnState(int theConnState);					//New Serial connection state
	void SignalNewParams();										//Signal that indicates to the widget that params have been externally updated
	void SignalDatasetStart();									//A new dataset (one spectrum, multiple spectra or signal-time) is starting
	void SignalDatasetEnd();									//Dataset has ended
	void SignalSpecStart();										//A new spectrum is starting (including a spectrum in a multi-spectrum dataset)
	void SignalSpecEnd();										//Spectrum ends
	void SignalMIDstart();										//A new MID scan is starting in a signal-time dataset
	void SignalMIDend();										//A MID dataset has ended

public:
	virtual void Start() = 0;									//Starts scanning
	void StopNow() { SetScanState(scanState_stopNow); }			//Requests the scan to be stopped as soon as possible
	void StopAtEnd() { SetScanState(scanState_stopAtEnd); }		//Requests the scaning to be stopped at end of scan
	virtual void PowerOn() = 0;
	virtual void PowerOff() = 0;
	void TriggerNewParams() { emit SignalNewParams(); }			//Emits the New Params signal

	int ScanState() const { return scanState; }
	int PowerState() const { return powerState; }

	bool IsInStandby() const { return scanState == scanState_standby; }
	bool IsScanning() const { return !IsInStandby(); }
	bool IsStopNowRequested() const { return scanState == scanState_stopNow; }
	bool IsStopAtEndRequested() const { return scanState == scanState_stopAtEnd; }
	bool IsPowerOn() const { return powerState == powerState_on; }
	bool IsConnected() const { return connState == connState_on; }

public:
	//In saveob
	int opMode;			//0 - mass spectrum, 1 - signal-time
	int detectorType;	//0 - SEM, 1 - Faraday cup
	
	double semVoltage;
	double emission;	//emission current in microamps
	int filament;		//Which filament to use, 1 or 2 where available

	double specStart;
	double specEnd;
	double specStep;
	int specMode;		//0 - single scan, 1 - keep scanning
	
	//Hiden-specific stuff
	bool fHidenAutoranging;	//Whether autoranging is used on hiden QMS
	int hidenCurRange;	//from -5 to -10, initial setting when autoranging
	int hidenMinRange;		//when autoranging, from -5 to -10
	int hidenMaxRange;		//when autoranging, from -5 to -10
	int hidenDwell;			//in percent
	int hidenSettleTime;	//in percent

	CHArray<double> massTable;		//The list of masses 
	
	BString portString;		//Port name, for example (and most likely) COM1

//Current state
protected:
	void SetScanState(int val) { scanState = val; emit SignalNewScanState(val); }
	void SetPowerState(int val) { powerState = val; emit SignalNewPowerState(val); }
	void SetConnState(int val) { connState = val; emit SignalNewConnState(val); }

protected:
	std::atomic<int> scanState;			//0 - standby, 1 - data acquisition, 2 - awaiting stop
	std::atomic<int> powerState;		//0 - off, 1 - on
	std::atomic<int> connState;			//Serial connection state, 0 - disconnected, 1 - connected
	CTimer timer;

//Port operation - for serial-connected QMSes
public:
	CHArray<BString> portList;			//List of ports identified by the EnumeratePorts
	CHArray<BString> portDescList;		//The descriptions provided for the ports in portList

protected:
	std::unique_ptr<serial::Serial> port;	//Serial port object used for all communication with the mass spec
	BString termToQms;						//Terminating characters on messages to QMS
	BString termFromQms;					//Terminating characters on messages from QMS
	std::recursive_mutex portMutex;		//The mutex guarding the port

public:
	virtual bool Connect(){ return true; }				//Returns true if connection is successful, false otherwise
	virtual BString GetIdString(){ return ""; }			//Queries for and returns the ID string from the mass spec over the serial port

	void EnumeratePorts();
	void Disconnect();
	void SetQmsPort(const BString& portName){portString = portName;}

	BString SendReceive(const BString& sendString);
	template<class T> BString SendReceive(const BString& formatString, const T& val);
};

inline void ExpDeviceQms::EnumeratePorts()
{
	std::lock_guard<std::recursive_mutex> lock(portMutex);
	std::vector<serial::PortInfo> enumeration = serial::list_ports();

	portList.Clear();
	portDescList.Clear();

	for (auto& item : enumeration)
	{
		portList << item.port;
		portDescList << item.description;
	}

	CHArray<int> perm;
	portList.SortPermutation(perm);
	portList.Permute(perm);
	portDescList.Permute(perm);
}

inline void ExpDeviceQms::Disconnect()
{
	std::lock_guard<std::recursive_mutex> lock(portMutex);
	if (port && port->isOpen()) port->close();
	port.reset();

	SetConnState(connState_off);
}

inline BString ExpDeviceQms::SendReceive(const BString& sendString)
{
	std::lock_guard<std::recursive_mutex> lock(portMutex);

	if (!port || !port->isOpen()) return "";

	emit SignalNewCom(sendString);

	//Write
	port->write(sendString + termToQms);

	//Read response
	BString response;
	int termLength = termFromQms.GetLength();
	while (1)
	{
		if (port->available())
		{
			while (port->available()) response += port->read();

			if (response.Right(termLength) == termFromQms) break;	//break on terminating sequence
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	BString result = response.Left(response.GetLength() - termLength);

	emit SignalNewCom(result);

	return result;
}

template <class T>
BString ExpDeviceQms::SendReceive(const BString& formatString, const T& val)
{
	BString str;
	str.Format(formatString, val);
	return SendReceive(str);
}