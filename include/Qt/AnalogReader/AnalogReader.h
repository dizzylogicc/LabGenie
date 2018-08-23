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

class AnalogReader : public ExpDevice
{
	Q_OBJECT

public:
	AnalogReader(xml_node& theDevNode, xml_node& theSaveNode, QObject* parent = 0) :
		ExpDevice(theDevNode, theSaveNode, parent)
	{
		fWidgetable = true;

		//devData
		devData.AddChildAndOwn("a", a);
		devData.AddChildAndOwn("b", b);

		//saveData
		saveData.AddChildAndOwn("xMin", xMin);
		saveData.AddChildAndOwn("xMax", xMax);
		saveData.AddChildAndOwn("yMin", yMin);
		saveData.AddChildAndOwn("yMax", yMax);
	}
	~AnalogReader() {}

public:
	virtual double ReadOnce()				//Does not emit signal new data
			{return b + a * InternalReadOnce();}

	virtual void StartContinuous() = 0;				//In continuous mode, reader reads continuously and emits SignalNewData()
	virtual void StopContinuous() = 0;
	virtual double Period() = 0;					//Period between reads in the continuous mode, in seconds
	double Frequency() { return 1.0/Period(); }		//Inverse period, Hz

protected:
	virtual double InternalReadOnce() = 0;
	void EmitNewData(double data) { emit SignalNewData(b + a * data); }

signals:
	//Call EmitNewData instead of calling emit SignalNewData() directly
	void SignalNewData(double data);

protected:
	//Reader returns ax + b
	//By default, a = 1, b = 0
	double a = 1;
	double b = 0;

public:
	//The four chart limits
	//Added to saveData in constructor
	double xMin = 0;
	double xMax = 600;
	double yMin = 0;
	double yMax = 10;
};