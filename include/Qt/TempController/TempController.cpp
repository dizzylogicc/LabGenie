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

#include "TempController.h"
#include "QtUtils.h"

TempController::TempController(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent /*=0*/) :
ExpDevice(theDevNode, theSaveNode, parent),
reader(nullptr),
writer(nullptr),
tempIntegral(0),
tempDiffArr(10000),
tempArr(10000),
voltageArr(10000),
tpdBeginTime(0),
tpdEndTime(0)
{
	fReading = false;
	fControlling = false;

	timer.SetTimerZero(0);

	//Not in devData
	fWidgetable = true;

	//DevData
	devData.AddChildAndOwn("reader", readerName);
	devData.AddChildAndOwn("writer", writerName);

	//SaveData
	saveData.AddChildAndOwn("setpoint", params.setpoint);
	saveData.AddChildAndOwn("rate", params.rate);
	saveData.AddChildAndOwn("fRounded", params.fRounded);
	saveData.AddChildAndOwn("radius", params.radius);
	saveData.AddChildAndOwn("PIDprop", params.PIDprop);
	saveData.AddChildAndOwn("PIDintegral", params.PIDintegral);
	saveData.AddChildAndOwn("PIDderiv", params.PIDderiv);
	saveData.AddChildAndOwn("maxControlV", params.maxControlV);
	saveData.AddChildAndOwn("maxTemp", params.maxTemp);
	saveData.AddChildAndOwn("xMin", params.xMin);
	saveData.AddChildAndOwn("yMin", params.yMin);
	saveData.AddChildAndOwn("xMax", params.xMax);
	saveData.AddChildAndOwn("yMax", params.yMax);

	//Load all data
	Load();
}

//When new data is received from the reader
void TempController::OnNewData(double measured)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (!fReading) return;

	if (fControlling) FollowRamp(measured);
	else
	{
		emit SignalNewData(measured, StopwatchTime());
	}
}

void TempController::FollowRamp(double measured)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	double setpoint = ramp.InterpolatePoint(RampTime());
	emit SignalNewControlData(measured, setpoint, StopwatchTime());
	PID(measured, setpoint);
}

void TempController::PID(double measured, double setpoint)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	//Truncate the temp set point if it exceeds maxTemp
	if (setpoint > params.maxTemp) setpoint = params.maxTemp;

	double tempDiff = setpoint - measured;

	tempDiffArr.Append(tempDiff);

	if (tempDiffArr.Count() < 5) return;

	/////////////////////////////////////Calculating power
	tempIntegral += tempDiff * period;

	//Anti-windup
	double maxIntegral = params.maxControlV * params.maxControlV / params.PIDintegral;
	if (tempIntegral < 0) tempIntegral = 0;
	if (tempIntegral > maxIntegral) tempIntegral = maxIntegral;

	double derivative = ((tempDiffArr[0] - tempDiffArr[1]) +
		(tempDiffArr[0] - tempDiffArr[2]) / 2 +
		(tempDiffArr[0] - tempDiffArr[3]) / 3 +
		(tempDiffArr[0] - tempDiffArr[4]) / 4) / 4;
	
	derivative /= period;

	double proportional = tempDiff;

	double power =	params.PIDintegral * tempIntegral
					+ params.PIDprop * proportional
					+ params.PIDderiv * derivative;

	if (power<0) power = 0;
	double daqVoltage = sqrt(power);

	if (daqVoltage > params.maxControlV) daqVoltage = params.maxControlV;

	tempArr.Append(measured);
	voltageArr.Append(daqVoltage);

	writer->WriteOnce(daqVoltage);
}

void TempController::SetAveragePower(int num)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	writer->WriteOnce(voltageArr.Average(num));
}

void TempController::SetZeroPower()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	ClearPID();
	writer->WriteOnce(0);
}

bool TempController::fTempWithin(double T, double range, int numReads)
//returns true if temp is within +- range of T within a given number of points in tempArr
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (numReads > tempArr.Count()) return false;

	bool result = true;
	for (int counter1 = 0; counter1<numReads; counter1++)
	{
		if ((tempArr[counter1]<(T - range)) || (tempArr[counter1]>(T + range)))
		{
			result = false; break;
		}
	}

	return result;
}

double TempController::CreateRamp(double targetTemp, double rate,
	bool useSmoothing/*=false*/, double smoothingWidth/*=1*/)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	double curTime = RampTime();
	double curTemp = Temp();

	ramp.RemoveAllPointsAfter(curTime);

	double tempDiff = abs(targetTemp - curTemp);
	double heatUpEndTime = curTime + tempDiff / rate;
	double totalTime = heatUpEndTime - curTime;

	ramp.AddPoint(curTime, curTemp);

	if (!useSmoothing || smoothingWidth <= 0)		//No smooth transition between the ramp and the flat portion
	{
		ramp.AddPoint(heatUpEndTime, targetTemp);
	}
	else //Create a smooth ramp (a regular ramp smoothed by averaging around the transition)
	{
		//make sure that the smoothingWidth is at least twice as large as the ramp time
		if (smoothingWidth > totalTime * 2) smoothingWidth = totalTime * 2;

		//Add 30 points for the transition
		int numTransPoints = 30;
		double tIncrement = smoothingWidth / (numTransPoints - 1);
		double rateSign = 1;
		if (curTemp > targetTemp) rateSign = -1;

		for (int i = 0; i < numTransPoints; i++)
		{
			double t = heatUpEndTime - smoothingWidth / 2 + tIncrement*i;

			//Make sure we don't add a point that has the same value as the one already added
			if (i == 0 && t == curTime) continue;

			ramp.AddPoint(t, SmoothRampFunction(t, heatUpEndTime, targetTemp, rateSign*rate, smoothingWidth));
		}
	}

	return heatUpEndTime;
}

// Smooth transition function for the approach to the Tp
// Derived from the linear ramp smoothed by averaging around the transition point
// t is the time at which the smoothed function is requested, in seconds
// r is the ramping rate, in K/s
// Tp is the final temperature reached by the ramp, in K
// tp is the time at which Tp would be reached if it was a linear ramp, in seconds
// w is the width of the transition region, in seconds (the averaging width)
double TempController::SmoothRampFunction(double t, double tp, double Tp, double r, double w)
{
	//The formula is (8 Tp w - r (-2 t + 2 tp + w)^2)/(8 w)

	double val = (2*tp - 2*t + w);
	val *= val;

	return (8*Tp*w - r*val) / (8*w);
}

//Adds a point to ramp
void TempController::AddRampPoint(double time, double temp)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	ramp.RemoveAllPointsAfter(time);
	ramp.AddPoint(time, temp);
}

//Creates a normal TPD profile: ramps to "from" temp, optionally delay start for temp stabilization,
//then ramp to the "to" temp and stay there after the end of the TPD
//Sets the tpdStartTime and tpdEndTime variables which can then be requested by the routine
//That runs the TPD
void TempController::CreateTPDprofile(double from, double to, double rate, double delay,
	bool useSmoothing, double smoothingWidth)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	double fromReachedTime = CreateRamp(from, rate, useSmoothing, smoothingWidth);
	tpdBeginTime = fromReachedTime + delay;
	AddRampPoint(tpdBeginTime, from);

	tpdEndTime = tpdBeginTime + abs(from - to) / rate;
	AddRampPoint(tpdEndTime, to);
}


//Creates an isothermal TPD profile:
//Ramps to the "from" temp with the given rate, then waits there for the specified duration
//TPD starts when the temp reaches the "from" value
//Sets they tpdStartTime and tpdEndTime
void TempController::CreateTPDprofileIsothermal(double from, double rate, double duration,
	bool useSmoothing, double smoothingWidth)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	double fromReachedTime = CreateRamp(from, rate, useSmoothing, smoothingWidth);
	tpdBeginTime = fromReachedTime;

	tpdEndTime = tpdBeginTime + duration;
	AddRampPoint(tpdEndTime, from);
}