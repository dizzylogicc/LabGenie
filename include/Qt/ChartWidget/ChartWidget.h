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

#include <QWidget>
#include <QtCharts>

#include "CwLineGroup.h"
#include "CwEditGroup.h"
#include "Data.h"
#include "QDoubleEdit.h"
#include "QtUtils.h"

#include "ui_ChartWidget.h"

class CwOverlay : public QWidget
{
public:
	CwOverlay(QWidget* parent, QRect rectangle, QPoint theCoord,
					const BString& theXtext, const BString& theYtext ) :
	QWidget(parent),
	coord(theCoord),
	xText(theXtext),
	yText(theYtext)
	{
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_TransparentForMouseEvents);

		resize(rectangle.width(), rectangle.height());
		QPoint topLeft = rectangle.topLeft();
		move(topLeft);

		//Account for the coordinate shift due to move() 
		overlayRect = QRect(rectangle.topLeft() - topLeft, rectangle.bottomRight() - topLeft);
		coord -= topLeft;
	}

protected:
	void paintEvent(QPaintEvent* event);

private:
	QPoint coord;			//Screen coordinates at the center point
	QRect overlayRect;		//Rectangle over which the overlay is drawn
	BString xText, yText;	//the text for X and Y positions
};

class ChartWidget : public QWidget
{
	Q_OBJECT

public:
	ChartWidget(int numLines, QWidget* parent = 0);
	~ChartWidget(){}

public slots:
	void MinXenterPressed() { minX.EnterPressed(); }
	void MaxXenterPressed() { maxX.EnterPressed(); }
	void MinYenterPressed() { minY.EnterPressed(); }
	void MaxYenterPressed() { maxY.EnterPressed(); }

public:
	//Set scientific or decimal notation on axes (true - scientific, false - decimal)
	void SetSciNotationX(bool val) { SetSciNotation(val, axisX, notationStringX); }
	void SetSciNotationY(bool val) { SetSciNotation(val, axisY, notationStringY); }
	void SetNotationX(const BString& format) { SetNotation(format, axisX, notationStringX); }
	void SetNotationY(const BString& format) { SetNotation(format, axisY, notationStringY); }

	//These functions set the limits without further checks
	void SetXmin(double val) { minX.SetVal(val); }
	void SetXmax(double val) { maxX.SetVal(val); }
	void SetYmin(double val) { minY.SetVal(val); }
	void SetYmax(double val) { maxY.SetVal(val); }

	//Query the current min max values
	double xMin() const { return minX.Val(); }
	double xMax() const { return maxX.Val(); }
	double yMin() const { return minY.Val(); }
	double yMax() const { return maxY.Val(); }

	//Set text on the axes
	void SetXaxisText(const BString& text) { ui.labelX->setText(QString(text)); }
	void SetYaxisText(const BString& text) { ui.labelY->setText(QString(text)); }

	//Returns the array of all line series
	CHArray<QLineSeries*> GetAllSeries() const
	{
		CHArray<QLineSeries*> series(lines.Count());
		for (auto& cur : lines) series << cur.series;
		return series;
	}

	//Showing and hiding the last value and deviation frames
	void SetHiddenLastValue(bool fHidden){ ui.frameLastValue->setHidden(fHidden); }
	void SetHiddenDeviation(bool fHidden){ ui.frameDeviation->setHidden(fHidden); }

	//Setting the value for Last Value and Deviation fields
	//And clearing the text from those fields
	void SetLastValue(double val) { editLastValue.SetVal(val); }
	void SetDeviation(double val) { editDeviation.SetVal(val); }
	void ClearLastValue() { editLastValue.ClearEdit(); }
	void ClearDeviation() { editDeviation.ClearEdit(); }

	//Access to line groups
	int LineCount() const { return lines.Count(); }
	CwLineGroup& Line(int index) const { return lines[index]; }
	CHArray<CwLineGroup>& GetLines() { return lines; }

	//Add a line group - returns the index of the line
	int AddLine()
	{
		lines << CwLineGroup();
		int index = lines.Count() - 1;
		CwLineGroup& cur = lines[index];

		CreateSeries(index);
		return index;
	}

	//Clear all lines
	void ClearAll()	{ for (auto& cur : lines) cur.Clear(); }

	//Minimum and maximum values for all the data series on X and Y axes
	double DataXmax() { return DataLimit(true, false); }
	double DataXmin() { return DataLimit(true, true); }
	double DataYmax() { return DataLimit(false, false); }
	double DataYmin() { return DataLimit(false, true); }

	CData GetLineData(int lineNum)
	{
		CData result;
		if (lineNum < 0 || lineNum >= LineCount()) return result;

		result.xArr = Line(lineNum).xArray;
		result.yArr = Line(lineNum).yArray;
		return result;
	}

	void SetLineData(const CData& newData, int lineNum)	{ lines[lineNum].SetData(newData); }

	CHArray<QColor> GetLineColors()
	{
		CHArray<QColor> result(lines.Count());
		for (int i = 0; i < lines.Count(); i++) result << lines[i].Color();
		return result;
	}

protected:
	//Event filter to catch mouse clicks on QChart plot area
	bool eventFilter(QObject *obj, QEvent *event);

private:
	double DataLimit(bool fXaxis, bool fMin);
	void SetSciNotation(bool val, QValueAxis* axis, BString& theNotString);
	void SetNotation(const BString& format, QValueAxis* axis, BString& theNotString);

	void UpdateEdits();

	//Tests whether a string represents a number - uses a regex
	bool IsNumber(const BString& str);

	//Creates a series from a pointer, attaches it to graph and axes, assigns standard color, and sets the pen
	void CreateSeries(int index, bool fAttachAxes = true);

private:
	CwEditGroup minX, maxX, minY, maxY;

private:
	QDoubleEdit editLastValue, editDeviation;

private:
	BString notationStringX;
	BString notationStringY;

	BString decNotationString;
	BString sciNotationString;

private:
	QChart* chart;
	QChartView* chartView;
	QLineSeries* defaultSeries;
	QValueAxis* axisX;
	QValueAxis* axisY;
	CHArray<CwLineGroup> lines;
	CHArray<QRgb> stdColors;

private:
	//Limit array - avoid reallocating each time
	CHArray<double> limArray;

//Handling the overlay triggered by mouse clicks on the plot area - show click coordinates
//Or coordinates of the closest point
private:
	void CreateOverlayClick(QPoint coord);		//create overlay showing the position of the click
	void CreateOverlayPoint(QPoint coord);		//create overlay showing the position of the closest data point
	void ShowOverlay(QPoint coord, QPointF val);
	void RemoveOverlay();
	
	QPointF CoordToValue(QPoint coord);
	QPoint ValueToCoord(QPointF val);
	bool IsInPlotArea(double x, double y);
	QWidget* overlay;
	bool fRightPressed, fLeftPressed;		//Variables that respond to left of right mouse button presses

private:
	Ui::ChartWidgetClass ui;
};