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

#ifdef WITH_NI_HARDWARE

#include "NiDaqAnalogReader.h"

bool NiDaqAnalogReader::Initialize(StdMap<BString, ExpDevice*>& devMap)
{
	buffer.Resize(averagingSamples, true);

	//Identify the acquisition mode
	int32 acMode;
	if (mode == "Diff")	acMode = DAQmx_Val_Diff;
	else if (mode == "RSE")	acMode = DAQmx_Val_RSE;
	else if (mode == "NRSE") acMode = DAQmx_Val_NRSE;
	else if (mode == "PseudoDiff")	acMode = DAQmx_Val_PseudoDiff;
	else
	{
		EmitError(BString("Unknown acquisition mode, ") + mode + ". Mode should be one of: Diff, PseudoDiff, RSE, NRSE.");
		return false;
	}

	//Create the task
	if (taskHandle != 0) DAQmxClearTask(taskHandle);

	int32 status;
	DAQmxCreateTask("", &taskHandle);

	BString channelString;
	channelString.Format("ai%i", channel);

	BString fullChannel = NIdeviceName + "/" + channelString;
	status = DAQmxCreateAIVoltageChan(taskHandle, fullChannel, "", acMode, minVolt, maxVolt, DAQmx_Val_Volts, NULL);
	if (status != 0)	//Something's not right
	{
		if (status > 0) EmitError("Warning received when creating an analog voltage reading channel.");
		else if (status < 0)
		{
			EmitError("Error creating an analog voltage reading channel.");
			return false;
		}
	}

	DAQmxCfgSampClkTiming(taskHandle, "", samplingRate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, averagingSamples);
	if (status != 0)	//Something's not right
	{
		if (status > 0) EmitError("Warning received when configuring timing on an analog voltage reading channel.");
		else if (status < 0)
		{
			EmitError("Error configuring timing on an analog voltage reading channel.");
			return false;
		}
	}

	return true;
}


double NiDaqAnalogReader::InternalReadOnce()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	int32 numRead = 0;
	DAQmxStartTask(taskHandle);
	DAQmxReadAnalogF64(taskHandle, averagingSamples, 0.5, DAQmx_Val_GroupByChannel,
		buffer.arr, (uInt32)buffer.Size(), &numRead, NULL);
	DAQmxStopTask(taskHandle);

	return buffer.Mean();
}

//Thread that continuously acquires measurements with specified period
void NiDaqAnalogReader::AcquisitionThread()
{
	fThreadRunning = true;

	//Period in microseconds
	auto microsPeriod = std::chrono::microseconds(int(period * 1000000));

	while (1)
	{
		if (!fContinuousOn) break;

		auto startTime = std::chrono::system_clock::now();
		double result = InternalReadOnce();
		EmitNewData(result);
		std::this_thread::sleep_until(startTime + microsPeriod);
	}

	fThreadRunning = false;
}

#endif //WITH_NI_HARDWARE