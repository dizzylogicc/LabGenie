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

#include "HwPhidgets1048.h"

bool HwPhidgets1048::AddReader(TempReader1048* newReader)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	int channel = newReader->channel;
	if (channel < 0 || channel > 3)
	{
		EmitError("Channel parameter should be between 0 and 3 inclusive.");
		return false;
	}

	BString tcType = newReader->tcType;
	if (tcType != "K" && 
		tcType != "J" &&
		tcType != "E" &&
		tcType != "T")
	{
		EmitError("tcType parameter should be one of: K, J, E or T.");
		return false;
	}

	if (!readers[channel].IsEmpty())
	{
		if (readers[channel][0]->tcType != tcType)
		{
			BString chString;
			chString.Format("%i", channel);
			EmitError("tcType parameter does not match other"
				" thermocouple types already attached to channel "+ chString);
			return false;
		}
	}

	readers[channel] << newReader;

	return true;
}

bool HwPhidgets1048::Start()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (fRunning) return true;

	handle = 0;
	CPhidgetTemperatureSensor_create(&handle);	//create the Analog object
	baseHandle = (CPhidgetHandle)handle;

	if (!HwPhidgets::Start()) return false;

	//Set the thermocouple types for all channels on which there are readers attached
	for (int i = 0; i < 4; i++)
	{
		if (readers[i].IsEmpty()) continue;

		BString tcType = readers[i][0]->tcType;
		CPhidgetTemperatureSensor_ThermocoupleType pType;
		if (tcType == "K")			pType = PHIDGET_TEMPERATURE_SENSOR_K_TYPE;		//73K to 1520K
		else if (tcType == "J")		pType = PHIDGET_TEMPERATURE_SENSOR_J_TYPE;
		else if (tcType == "E")		pType = PHIDGET_TEMPERATURE_SENSOR_E_TYPE;
		else if (tcType == "T")		pType = PHIDGET_TEMPERATURE_SENSOR_T_TYPE;
		else
		{
			EmitError("Unknown thermocouple type during device start.");
			return false;
		}

		CPhidgetTemperatureSensor_setThermocoupleType(handle, i, pType);
	}

	fRunning = true;

	std::thread acqThread(&HwPhidgets1048::AcquisitionThread, this);
	acqThread.detach();

	return true;
}

bool HwPhidgets1048::Stop()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (!fRunning) return true;
	fRunning = false;
	return HwPhidgets::Stop(); 
}

void HwPhidgets1048::AcquisitionThread()
{
	while (fRunning)
	{
		//Read the temperatures and distribute them to readers
		ReadTemps();
		//Sleep until the next cycle
		std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
	}
}

void HwPhidgets1048::ReadTemps()
{
	//The routine that polls the phidgets device and distributes the new data to readers
	std::lock_guard<std::recursive_mutex> lock(mutex);

	//Read temperature on all channels where there are readers attached
	for (int i = 0; i < 4; i++)
	{
		if (readers[i].IsEmpty()) continue;
		
		double temperature;
		CPhidgetTemperatureSensor_getTemperature(handle, i, &temperature);

		//Phidget device returns temperature in degrees celcius
		//Convert it to Kelvin
		temperature += 273.15;

		for (auto& curReader : readers[i]) curReader->UpdateReading(temperature);
	}
}