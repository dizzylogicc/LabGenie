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
#include "Qt/TempController/TempController.h"
#include "Qt/Qms/ExpDeviceQms.h"

#define tpdState_idle 0
#define tpdState_running 1
#define tpdState_finished 2

struct TpdChartPoint
{
	TpdChartPoint(){}

	TpdChartPoint(int theIndex, double theTempOrTime, double theSignal) :
	index(theIndex), tempOrTime(theTempOrTime), signal(theSignal) {}

	int index;			//The index of the mass in the mass table
	double tempOrTime;
	double signal;
};

class ExpDeviceTpd : public ExpDevice
{
	Q_OBJECT

public:
	ExpDeviceTpd(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0);
	~ExpDeviceTpd(){}

signals:
	void SignalNewState(int state);
	void SignalNewTpdData(TpdChartPoint point);
	void SignalResetLineData(CData newData, int index);
	void SignalDataWritten();

public slots:
	void OnNewTempData(double measured, double setpoint, double stopwatchTime);
	void OnNewQmsData(CHArray<QmsDataPoint> newData);

public:
	double TpdDuration() { return tpdEndTime - tpdBeginTime; }
	void StartTpd();
	void StopTpd();
	void WriteData();
	void SetTempControl(TempController* newTempControl){ tempControl = newTempControl; }
	void SetQms(ExpDeviceQms* newQms) { qms = newQms; }
	TempController* TempControlDev(){ return tempControl; }

	int State() { return state; }
	void SetState(int newState) { state = newState; EmitState(); }
	void EmitState() { emit SignalNewState(state); }

private:
	void CheckForEndCondition(double curTime);

public:
	//Pure virtual functions to be redefined in all devices
	virtual void Dependencies(CHArray<BString>& outList){ outList << tempControlName << qmsName; }
	virtual bool Initialize(StdMap<BString, ExpDevice*>& devMap);
	virtual void PostInitialize(){};
	///////////////////////////////////////////////////////

private:
	TempController* tempControl;
	ExpDeviceQms* qms;

	//Saveob
	BString tempControlName;
	BString qmsName;

	int state;

	//These store the TPD data and need to be cleared on every TPD launch
	CData tempTimeData;
	CHArray<CData> qmsTimeData;
	CHArray<CData> qmsTempData;
	CHArray<double> massTable;

	double tpdBeginTime;
	double tpdEndTime;
	bool fDataIsothermal;		//whether the current data was acquired isothermally or in standard TPD

public:
	double tempFrom;
	double tempTo;
	double rate;
	double delay;
	bool fIsothermal;
	double duration;
	BString folder;

	//Not in saveob
	int expNumber;
};