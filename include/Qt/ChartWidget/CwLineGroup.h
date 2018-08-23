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

#include <QtCharts>
#include "ArrayMinMax.h"
#include "Data.h"

class CwLineGroup
{
public:
	CwLineGroup() :
		series(nullptr)
	{
		maxChartPoints = 10000;
		skipInterval = 1;
		skipCounter = 1;
	}

public:
	void AddPoint(double x, double y)
	{
		xArray.AddPointMinMax(x);
		yArray.AddPointMinMax(y);

		if (series->count() >= maxChartPoints) Redraw();

		//Add a point only if we have already skipped as many points as required by skipInterval
		if (skipCounter >= skipInterval)
		{
			series->append(x, y);
			skipCounter = 1;
		}
		else skipCounter++;
	}

	void SetData(const CData& newData)
	{
		Clear();
		for (int i = 0; i < newData.Count(); i++)
		{
			xArray.AddPointMinMax(newData.xArr[i]);
			yArray.AddPointMinMax(newData.yArr[i]);
		}
		Redraw();
	}
	
	int Count() const { return xArray.Count(); }
	bool IsEmpty() const { return Count() == 0; }

	void Clear()
	{
		xArray.Clear();
		yArray.Clear();
		if(series) series->clear();

		skipInterval = 1;
		skipCounter = 1;
	}

	void CopyFromLine(CwLineGroup& otherLine)
	{
		Clear();
		xArray = otherLine.xArray;
		yArray = otherLine.yArray;

		Redraw();
	}

	void Redraw()
	{
		series->clear();

		skipInterval = 2 * Count() / maxChartPoints + 1;
		skipCounter = 1;

		for (int i = 0; i < Count(); i += skipInterval) series->append(xArray[i], yArray[i]);
	}

	void SetVisible(bool fVisible) { series->setVisible(fVisible); }

	void SetColor(QColor color) { series->setColor(color); }
	QColor Color() const { return series->color(); }

	double MinX() const { return xArray.CurMin(); }
	double MinY() const { return yArray.CurMin(); }
	double MaxX() const { return xArray.CurMax(); }
	double MaxY() const { return yArray.CurMax(); }

	//Returns the limit for specified axis and specified side
	double GetLimit(bool fXaxis, bool fMin)
	{
		if (fXaxis)
		{
			if (fMin) return MinX();
			else return MaxX();
		}
		else
		{
			if (fMin) return MinY();
			else return MaxY();
		}
	}

public:
	QLineSeries* series;
	CHArrayMinMax<double> xArray;
	CHArrayMinMax<double> yArray;

private:
	//Maximum number of points to be plotted on screen
	//QChart does not like too many points (in excess of 20,000 or so)
	//Points are still saved in the data, but not shown on screen
	int maxChartPoints;

	//How many points to skip before showing a point on screen
	//Depends on the number of points present
	int skipInterval;

	//Number of points skipped in a current skip cycle
	int skipCounter;
};