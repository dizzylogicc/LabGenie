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
#include "BString.h"
#include <QLineEdit>
#include <QtCharts>

class CwEditGroup
{
public:
	CwEditGroup() :
		numberRegexp("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?")
	{
		fInitialized = false;
	}

public:
	void SetGroup(QLineEdit* theEdit, BString* theFormat, QValueAxis* theAxis, bool theFmin, CwEditGroup* theOtherGroup)
	{
		edit = theEdit;
		format = theFormat;
		axis = theAxis;
		fMin = theFmin;
		otherGroup = theOtherGroup;
	}

	double Val() const { return val; }

	void SetVal(double theVal, bool fUpdateOther = true)
	{
		if (theVal == val && fInitialized) return;

		fInitialized = true;
		val = theVal;
		UpdateEdit();
		if (fMin) axis->setMin(val);
		else axis->setMax(val);
		if (fUpdateOther) otherGroup->FromAxis();
	}

	void EnterPressed()
	{
		BString str = edit->text().toStdString();

		if (IsNumber(str))
		{
			double num = atof(str);
			bool fAccept = false;
			if (fMin) fAccept = (num < otherGroup->Val());
			else fAccept = (num > otherGroup->Val());

			if (fAccept) { SetVal(num); return; }
		}

		//The string is not a number
		UpdateEdit();
	}

	void UpdateEdit()
	{
		BString str;
		str.Format(*format, val);

		//Remove stupid extra zeros from the scientific notation
		char lastFormatSymbol = (*format)[format->GetLength() - 1];
		if (lastFormatSymbol == 'e' || lastFormatSymbol == 'E')
		{
			str.Replace("E", "e");
			int plus = str.Replace("e+", "e");
			int minus = str.Replace("e-", "e");
			str.Replace("e0", "e");
			str.Replace("e0", "e");

			if (str[str.GetLength() - 1] == 'e') str += "0";

			if (plus > 0) str.Replace("e", "e+");
			else if (minus > 0) str.Replace("e", "e-");
		}

		edit->setText(QString(str));
	}

private:
	bool IsNumber(const BString& str)
	{
		return numberRegexp.exactMatch(QString(str));
	}

	//Copy the value from the axis to the group - in case the axis value was re-adjusted by the chart itself
	void FromAxis()
	{
		double axisVal;
		if (fMin) axisVal = axis->min();
		else axisVal = axis->max();

		SetVal(axisVal, false);
	}

private:
	double val;
	bool fInitialized;
	QLineEdit* edit;
	BString* format;
	QValueAxis* axis;
	bool fMin;
	CwEditGroup* otherGroup;	//Other group on the same axis - for comparison when changing min / max

private:
	QRegExp numberRegexp;
};