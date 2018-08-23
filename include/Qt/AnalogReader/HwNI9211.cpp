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

#include "HwNI9211.h"
#include "QtUtils.h"

HwNI9211::HwNI9211(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent) :
ExpDevice(theDevNode, theSaveNode, parent),
tcParams(8),
readers(4, true)
{
	//Not in saveob
	taskHandle = 0;
	fRunning = false;

	devData.AddChildAndOwn("NIdeviceName", NIdeviceName);

	//Parameters for each thermocouple type
	tcParams	<< TCparams9211("J", 63.15, 1473, DAQmx_Val_J_Type_TC)
				<< TCparams9211("K", 73.15, 1645, DAQmx_Val_K_Type_TC)
				<< TCparams9211("N", 73.15, 1573, DAQmx_Val_N_Type_TC)
				<< TCparams9211("R", 223.15, 2041, DAQmx_Val_R_Type_TC)
				<< TCparams9211("S", 223.15, 2041, DAQmx_Val_S_Type_TC)
				<< TCparams9211("T", 73.15, 673.15, DAQmx_Val_T_Type_TC)
				<< TCparams9211("B", 523.15, 2093, DAQmx_Val_B_Type_TC)
				<< TCparams9211("E", 73.15, 1273, DAQmx_Val_E_Type_TC);

	//Add thermocouple types to the map
	for (auto& cur : tcParams) tcParamMap << cur.tcType;

	//Load all data
	Load();
}

int32 CVICALLBACK HwNI9211::DataReadyCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *pObject)
{
	HwNI9211& ob = *((HwNI9211*)pObject);

	{
		std::lock_guard<std::recursive_mutex> lock(ob.mutex);
		if (!ob.fRunning) return 0;

		int32       numRead = 0;
		float64     data[4];

		DAQmxReadAnalogF64(taskHandle, 1, 10.0, DAQmx_Val_GroupByScanNumber, data, 4, &numRead, NULL);

		//Update all readers for all channels
		for (int i = 0; i < 4; i++)
		{
			for (auto& reader : ob.readers[i])
			{
				double val = data[reader->channel];
				reader->UpdateReading(val);
			}
		}

		return 0;
	}
}

//Starts the scanning and data distribution, if not running already
bool HwNI9211::Start()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	if (fRunning || NumOccupiedChannels() == 0) return false;

	//Create the task that will call the DataReadyCallback with new data
	
	double rate = SampleRate();		//Maximum permissible rate
	int numSamples = 1;

	DAQmxCreateTask("", &taskHandle);

	//Create up to four channels
	for (int i = 0; i < 4; i++)
	{
		//If no readers are attached to the channel, proceed to the next channel
		if (readers[i].IsEmpty()) continue;

		//We only need to create a channel for the first attached reader
		TempReaderNI9211* reader = readers[i][0];
		TCparams9211& curTC = tcParams[tcParamMap.GetIndex(reader->tcType)];

		BString channel;
		channel.Format("%s/ai%i", NIdeviceName, i);

		int result = DAQmxCreateAIThrmcplChan(taskHandle, channel, "", curTC.minTemp, curTC.maxTemp,
			DAQmx_Val_Kelvins, curTC.desig, DAQmx_Val_BuiltIn, 293, "");

		if (result != 0) EmitError("Could not create temperature reading channel for NI 9211 device "
								+ NIdeviceName + ".");
				
	}

	DAQmxCfgSampClkTiming(taskHandle, "", rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, numSamples);

	DAQmxRegisterEveryNSamplesEvent(taskHandle, DAQmx_Val_Acquired_Into_Buffer, numSamples, 0, HwNI9211::DataReadyCallback, this);

	DAQmxStartTask(taskHandle);

	fRunning = true;
	return true;
}

//Stops scanning
bool HwNI9211::Stop()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	if (!fRunning) return false;
	
	if (taskHandle != 0)
	{
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		taskHandle = 0;
	}

	fRunning = false;
	return true;
}

bool HwNI9211::AddReader(TempReaderNI9211* newReader)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	int channel = newReader->channel;
	if (channel < 0 || channel > 3)
	{
		EmitError("Channel parameter should be between 0 and 3 inclusive.");
		return false;
	}

	BString tcType = newReader->tcType;
	if (!tcParamMap.IsPresent(tcType))
	{
		EmitError("tcType parameter should be one of: J, K, N, R, S, T, B or E.");
		return false;
	}

	if (!readers[channel].IsEmpty())
	{
		if (readers[channel][0]->tcType != tcType)
		{
			EmitError("tcType parameter does not match other"
							" thermocouple types already attached to that channel.");
			return false;
		}
	}

	readers[channel] << newReader;
	return true;
}

#endif //WITH_NI_HARDWARE