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

#include "ExpDeviceTpd.h"
#include "Matrix.h"


Q_DECLARE_METATYPE(CHArray<QmsDataPoint>)

ExpDeviceTpd::ExpDeviceTpd(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent) :
ExpDevice(theDevNode, theSaveNode, parent)
{
	//Not in saveob
	fWidgetable = true;

	//DevData
	devData.AddChildAndOwn("tempControl", tempControlName);
	devData.AddChildAndOwn("qms", qmsName);

	//SaveData
	saveData.AddChildAndOwn("tempFrom", tempFrom);
	saveData.AddChildAndOwn("tempTo", tempTo);
	saveData.AddChildAndOwn("rate", rate);
	saveData.AddChildAndOwn("delay", delay);
	saveData.AddChildAndOwn("fIsothermal", fIsothermal);
	saveData.AddChildAndOwn("duration", duration);
	saveData.AddChildAndOwn("folder", folder);

	//initial state
	state = tpdState_idle;

	//Load all data
	Load();
}

bool ExpDeviceTpd::Initialize(StdMap<BString, ExpDevice*>& devMap)
{
	TempController* t = dynamic_cast<TempController*>(devMap[tempControlName]);
	if (t) SetTempControl(t);
	else return false;

	ExpDeviceQms* q = dynamic_cast<ExpDeviceQms*>(devMap[qmsName]);
	if (q) SetQms(q);
	else return false;

	//Connect to the new data signals from temp controller and qms
	qRegisterMetaType<CHArray<QmsDataPoint>>();
	QObject::connect(tempControl, &TempController::SignalNewControlData, this, &ExpDeviceTpd::OnNewTempData, Qt::QueuedConnection);
	QObject::connect(qms, &ExpDeviceQms::SignalNewData, this, &ExpDeviceTpd::OnNewQmsData, Qt::QueuedConnection);

	return true;
}

void ExpDeviceTpd::StartTpd()
{
	if (state == tpdState_running) return;

	//We will only start the TPD if the QMS is scanning and in the signalTime mode
	if (qms->ScanState() != scanState_scanning || qms->opMode != opMode_sigTime)
	{
		QtUtils::ErrorBox("Unable to start TPD: the QMS should be scanning in Signal-Time mode.");
		return;
	}

	//The temp controller also needs to be in the controlling mode
	if (!tempControl->IsControlling())
	{
		QtUtils::ErrorBox("Unable to start TPD: the temp controller should be in control mode.");
		return;
	}

	//Set the local mass table
	massTable = qms->massTable;

	//Clear and resize all data
	qmsTimeData.Resize(massTable.Count(), true);
	for (auto& cur : qmsTimeData) cur.Clear();

	qmsTempData.Resize(massTable.Count(), true);
	for (auto& cur : qmsTempData) cur.Clear();

	tempTimeData.Clear();

	//Create the ramps
	if (fIsothermal) tempControl->CreateTPDprofileIsothermal(tempFrom, rate, duration);
	else tempControl->CreateTPDprofile(tempFrom, tempTo, rate, delay);

	tpdBeginTime = tempControl->GetTpdBeginTime();
	tpdEndTime = tempControl->GetTpdEndTime();

	SetState(tpdState_running);
}

void ExpDeviceTpd::StopTpd()
{
	if (state != tpdState_running) return;

	tempControl->ShutDown();
	SetState(tpdState_idle);
}

void ExpDeviceTpd::WriteData()
{
	if (state != tpdState_finished) return;

	BString fileName;
	BString extension = ".txt";
	fileName.Format("%s/tpd%i_", folder, expNumber);

	massTable.Write(fileName + "masses" + extension);
	tempTimeData.Write(fileName + "tempTime" + extension);

	//What data we will target, depending on acquisition mode
	CHArray<CData>* targetDataP;
	if (fDataIsothermal) targetDataP = &qmsTimeData;
	else targetDataP = &qmsTempData;
	CHArray<CData>& targetData = *targetDataP;

	CMatrix<double> result(targetData.Count() + 1, targetData[0].Count());
	result[0] = targetData[0].xArr;

	for (int i = 0; i < targetData.Count(); i++) result[i + 1] = targetData[i].yArr;

	if (fDataIsothermal) result.Write(fileName + "qmsTime" + extension);
	else result.Write(fileName + "qmsTemp" + extension);
}

void ExpDeviceTpd::OnNewTempData(double measured, double setpoint, double stopwatchTime)
{
	if (state != tpdState_running) return;

	double curTime = tempControl->RampTime();
	if (curTime < tpdBeginTime) return;			//TPD is running, but nothing to record yet

	if (curTime >= tpdBeginTime && curTime < tpdEndTime)
	{
		//We should record the data
		tempTimeData.AddPoint(curTime - tpdBeginTime, measured);
		return;
	}

	CheckForEndCondition(curTime);
}

void ExpDeviceTpd::OnNewQmsData(CHArray<QmsDataPoint> newData)
{
	if (state != tpdState_running) return;

	double curTime = tempControl->RampTime();
	if (curTime < tpdBeginTime) return;			//TPD is running, but nothing to record yet

	if (curTime >= tpdBeginTime && curTime < tpdEndTime)
	{
		//We should record the data
		double curTemp = tempControl->Temp();
		for (auto& point : newData)
		{
			//Fix the time in the data point
			point.time = curTime - tpdBeginTime;

			double tempOrTime;
			if (fIsothermal) tempOrTime = point.time;
			else tempOrTime = curTemp;

			int index = massTable.PositionOfClosest(point.mass);
			qmsTimeData[index].AddPoint(point.time, point.signal);
			
			//emit the signal for the real-time plot
			emit SignalNewTpdData(TpdChartPoint(index,tempOrTime,point.signal));
		}

		return;
	}

	CheckForEndCondition(curTime);
}

void ExpDeviceTpd::CheckForEndCondition(double curTime)
{
	if (curTime >= tpdEndTime)
	{
		//TPD has ended, process the data and change the state
		SetState(tpdState_finished);

		if (massTable.Count() == 0) return;

		//Save a separate variable to indicate what was the data obtained
		fDataIsothermal = fIsothermal;

		//Isothermal TPD
		if (fIsothermal)
		{
			//Interpolate all qms signals into the times of the first qms mass
			CHArray<double> times = qmsTimeData[0].xArr;

			for (int i = 1; i < qmsTimeData.Count(); i++) qmsTimeData[i] = qmsTimeData[i].InterpolateArray(times);

			//Reset all data in the chart
			for (int i = 0; i < qmsTimeData.Count(); i++) emit SignalResetLineData(qmsTimeData[i], i);
		}
		//Regular TPD
		else
		{
			//Interpolate the qmsTime data into qmsTemp data
			//We'll use the times for the first mass in the mass table to interpolate all other masses

			CHArray<double> times = qmsTimeData[0].xArr;
			CData newTempTime = tempTimeData.InterpolateArray(times);
			CHArray<double> temps = newTempTime.yArr;

			qmsTempData[0].yArr = qmsTimeData[0].yArr;
			qmsTempData[0].xArr = temps;

			for (int i = 1; i < qmsTimeData.Count(); i++)
			{
				CData newQmsTimeData = qmsTimeData[i].InterpolateArray(times);
				qmsTempData[i].yArr = newQmsTimeData.yArr;
				qmsTempData[i].xArr = temps;
			}

			//Reset all data in the chart
			for (int i = 0; i < qmsTempData.Count(); i++) emit SignalResetLineData(qmsTempData[i], i);
		}

		//Stop heating the sample at the end of the TPD
		tempControl->ShutDown();

		//Write the data and increment the experiment number
		WriteData();
		emit SignalDataWritten();

	} //end if curTime > tpdEndTime
}