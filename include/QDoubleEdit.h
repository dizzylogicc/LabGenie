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

#include <QObject>
#include <QLineEdit>
#include <QRegExp>
#include "BString.h"

//Class for a Qt line edit that only accepts double values

class QDoubleEdit : public QObject
{
	Q_OBJECT

public:
	QDoubleEdit(QLineEdit* theEdit, QObject* parent) :			//Cannot have default value for the parent
		QObject(parent)
	{
		Init();
		SetEdit(theEdit);
	}

	QDoubleEdit(QObject* parent = 0) :
		QObject(parent){ Init(); }

signals:
	void SignalValueChanged(double newVal);

public:
	void SetEdit(QLineEdit* theEdit)
	{
		if (theEdit == edit) return;
		if (edit)
		{
			QObject::disconnect(edit, &QLineEdit::returnPressed, this, &QDoubleEdit::FromText);
			QObject::disconnect(edit, &QLineEdit::editingFinished, this, &QDoubleEdit::FromText);
		}

		edit = theEdit;

		if (edit)
		{
			QObject::connect(edit, &QLineEdit::returnPressed, this, &QDoubleEdit::FromText);
			QObject::connect(edit, &QLineEdit::editingFinished, this, &QDoubleEdit::FromText);
		}

		UpdateEdit();
	}

	void SetSciFormat() { format = sciFormat; UpdateEdit(); }
	void SetFloatFormat() { format = floatFormat; UpdateEdit(); }
	void SetFormat(const BString& theFormat) { format = theFormat; UpdateEdit(); }
	BString Format() const { return format; }

	void SetReadOnly(bool theVal) { fReadOnly = theVal; }
	void SetEnabled(bool theVal) { if (edit) edit->setEnabled(theVal); }

	double Val() const { return val; }
	void SetVal(double theVal)
	{
		if (theVal == val && fInitialized) return;

		fInitialized = true;
		val = theVal;
		emit SignalValueChanged(val);
		UpdateEdit();
	}

	QLineEdit* Edit() const { return edit; }

	//Copy the value of the variable into the text in the edit field
	void UpdateEdit()
	{
		if (!edit) return;

		temp.Format(format, val);

		//Remove stupid extra zeros from the scientific notation
		char lastFormatSymbol = format[format.GetLength() - 1];
		if (lastFormatSymbol == 'e' || lastFormatSymbol == 'E')
		{
			temp.Replace("E", "e");
			int plus = temp.Replace("e+", "e");
			int minus = temp.Replace("e-", "e");
			temp.Replace("e0", "e");
			temp.Replace("e0", "e");

			if (temp[temp.GetLength() - 1] == 'e') temp += "0";

			if (plus > 0) temp.Replace("e", "e+");
			else if (minus > 0) temp.Replace("e", "e-");
		}

		edit->setText(QString(temp));
	}

	//Removes all text from the edit
	void ClearEdit() { if (!edit) return; edit->setText(""); fInitialized = false; }
	
protected:
	void Init()
	{
		edit = nullptr;

		numberRegexp.setPattern("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");

		sciFormat = "%.2e";
		floatFormat = "%.2f";
		format = floatFormat;

		val = 0;
		fInitialized = false;
		fReadOnly = false;
	}

	bool IsNumber(const BString& str)
	{
		return numberRegexp.exactMatch(QString(str));
	}

	void FromText()
	{
		if (fReadOnly) return;		//do nothing if the field is read only
		temp = edit->text().toStdString();

		if (IsNumber(temp)) SetVal(atof(temp));
		else UpdateEdit();	//The string is not a number - return to previous value
	}
	
protected:
	QLineEdit* edit;

	double val;
	bool fInitialized;
	bool fReadOnly;

	BString format;
	BString sciFormat;
	BString floatFormat;

	BString temp;		//temporary string - avoid reallocation on update calls

	QRegExp numberRegexp;
};