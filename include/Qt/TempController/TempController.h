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
#include "Qt/AnalogWriter/AnalogWriter.h"
#include "Qt/AnalogReader/AnalogReader.h"
#include "Data.h"
#include "CyclicArray.h"
#include "Timer.h"
#include <mutex>
#include "SaveobToXml.h"

#define pidState_standby		0
#define pidState_reading		1
#define pidState_controlling	2

struct TempControlParams
{
	TempControlParams(){}

	double setpoint = 280;
	double rate = 2;

	bool fRounded = false;
	double radius = 2;
	
	double PIDprop = 0;
	double PIDintegral = 0;
	double PIDderiv = 0;
	
	double maxControlV = 5;
	double maxTemp = 280;

	double xMin = 0;
	double xMax = 1000;
	double yMin = 280;
	double yMax = 1000;
};

class TempController : public ExpDevice
{
	Q_OBJECT

public:
	TempController(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0);
	~TempController(){}

public:
	//Pure virtual overrides
	virtual void Dependencies(CHArray<BString>& outList){ outList << readerName << writerName; }
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap)
	{
		AnalogReader* r = dynamic_cast<AnalogReader*>(devMap[readerName]);
		if (r) SetReader(r);
		else return false;

		AnalogWriter* w = dynamic_cast<AnalogWriter*>(devMap[writerName]);
		if (w) SetWriter(w);
		else return false;

		return true;
	}
	virtual void PostInitialize(){}

signals:
	void SignalNewData(double measured, double stopwatchTime);							//Emits data to controller owner
	void SignalNewControlData(double measured, double setpoint, double stopwatchTime);	//Emits data to controller owner
	void SignalNewState(int state);

public slots:
	void OnNewData(double measured);		//Receives data from the reader
	void OnClose(){ SetZeroPower();	ExpDevice::OnClose(); }

public:
	bool IsReading()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return fReading;
	}

	bool IsControlling()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return fControlling;
	}

	void SetReader(AnalogReader* newReader)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		reader = newReader;
		period = reader->Period();

		//Connect the reader to the new data slot
		QObject::connect(reader, &AnalogReader::SignalNewData, this, &TempController::OnNewData);
	}

	void SetWriter(AnalogWriter* newWriter)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		writer = newWriter;
	}

	void SetParams(const TempControlParams& theParams)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		params = theParams;
	}

	TempControlParams Params()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return params;
	}

	void SetReading(bool val)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		if (val == fReading) return;

		if (val) StartReading();
		else { StopReading(); SetControlling(false); }

		EmitState();
	}

	void SetControlling(bool val)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		if (val == fControlling) return;

		if (val)
		{
			if (fReading) StartControlling();
		}
		else StopControlling();

		EmitState();
	}

	void AddRampPoint(double time, double temp);

	void ShutDown() { SetControlling(false); }

	void ClearPID()		//Erases tempDiffArr - needed for derivative
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		tempIntegral = 0;

		tempDiffArr.Clear();
		voltageArr.Clear();
		tempArr.Clear();
	} 

	//Ramp from current temp to a given temp with the given rate
	double CreateRamp() { return CreateRamp(params.setpoint, params.rate, params.fRounded, params.radius); }
	double CreateRamp(double targetTemp, double rate, bool useSmoothing = false, double smoothingWidth = 1);

	//Creates a normal TPD profile: ramps to "from" temp, optionally delay start for temp stabilization,
	//then ramp to the "to" temp and stay there after the end of the TPD
	//Sets the tpdStartTime and tpdEndTime variables which can then be requested by the routine
	//That runs the TPD
	void CreateTPDprofile(double from, double to, double rate, double delay, bool useSmoothing, double smoothingWidth);
	//Will use the temp controller settings for smoothing and smoothing width
	void CreateTPDprofile(double from, double to, double rate, double delay)
	{
		CreateTPDprofile(from, to, rate, delay, params.fRounded, params.radius);
	}

	//Creates an isothermal TPD profile:
	//Ramps to the "from" temp with the given rate, then waits there for the specified duration
	//TPD starts when the temp reaches the "from" value
	//Sets they tpdStartTime and tpdEndTime
	void CreateTPDprofileIsothermal(double from, double rate, double duration, bool useSmoothing, double smoothingWidth);
	//Will use the temp controller settings for smoothing and smoothing width
	void CreateTPDprofileIsothermal(double from, double rate, double duration)
	{
		CreateTPDprofileIsothermal(from, rate, duration, params.fRounded, params.radius);
	}

	//After the ramp is created, the TPD routine can request the times that the TPD starts and ends
	//For synchronization with the mass spec
	double GetTpdBeginTime()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return tpdBeginTime;
	}

	double GetTpdEndTime()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return tpdEndTime;
	}

	double Temp()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return reader->ReadOnce();
	}

	double RampTime()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return timer.GetCurTime(0);
	}

	double StopwatchTime()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return timer.GetCurTime(1);
	}
	
	bool fTempWithin(double T, double range, int numReads);

	double GetLastTempDiff()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return tempDiffArr[0];
	}

	void ZeroStopwatchTimer()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		timer.SetTimerZero(1);
	}

	void SetAveragePower(int num);

private:
	//Smooth transition function for rounded transitions
	double SmoothRampFunction(double t, double tp, double Tp, double r, double w);
	void PID(double measured, double setpoint);
	void FollowRamp(double measuredTemp);
	void SetZeroPower();
	void EmitState()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		if (!fReading) emit SignalNewState(pidState_standby);
		else
		{
			if (!fControlling) emit SignalNewState(pidState_reading);
			else emit SignalNewState(pidState_controlling);
		}
	}

	void StartReading()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		fReading = true;
		reader->StartContinuous();
		timer.SetTimerZero(1);
	}

	void StopReading()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		fReading = false;
		reader->StopContinuous();
	}

	void StartControlling()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		fControlling = true;
		ClearPID();
		CreateRamp(params.setpoint, params.rate, params.fRounded, params.radius);
	}

	void StopControlling()
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		fControlling = false;
		SetZeroPower();
	}

private:
	AnalogWriter* writer;
	AnalogReader* reader;

private:
	//Dev data
	BString readerName;
	BString writerName;
	
	//In saveob
	TempControlParams params;

	//Not in saveob
	double tempIntegral;
	double period;	//update period for the reader

	CyclicArray<double> tempDiffArr;		//The difference for the PID algorithm
	CyclicArray<double> voltageArr;			//Needed for setting average output voltage
	CyclicArray<double> tempArr;			//Needed for verifying whether the temperature
											//Has been within a given deviation for a given time

	double tpdBeginTime;		//End and start times of the TPD, set by the two CreateTPDprofile() routines
	double tpdEndTime;

	CData ramp;
	CTimer timer;				//Two timers: RampTime() - timer 0, does not reset, and StopwatchTime() - timer 1, resets as needed

	bool fReading;
	bool fControlling;

	std::recursive_mutex mutex;		//Mutex that protects all private data
};