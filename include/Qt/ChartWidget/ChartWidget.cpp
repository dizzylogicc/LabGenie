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

#include "ChartWidget.h"
#include <QRegExp>


ChartWidget::ChartWidget(int numLines, QWidget* parent /*= 0*/) : 
lines(numLines, true),
limArray(numLines),
stdColors(10),
overlay(nullptr)
{
	ui.setupUi(this);

	//Install event filter to catch mouse clicks on chart plot area
	qApp->installEventFilter(this);

	//mouse button variables
	fLeftPressed = false;
	fRightPressed = false;

	//Colors for the line series
	stdColors << QRgb(0xE00000) << QRgb(0x0C00F7) << QRgb(0x00E308)
		<< QRgb(0xE300C1) << QRgb(0x7100E3) << QRgb(0x00C798)
		<< QRgb(0x8FD600) << QRgb(0xE8AB02) << QRgb(0xFF00F7) << QRgb(0x00D9E8);

	chart = new QChart();
	chart->legend()->hide();

	//Add a dummy line series so that the default axes can be correctly created
	defaultSeries = new QLineSeries;
	chart->addSeries(defaultSeries);

	//Create and add lines to chart in reverse order
	//So that the first one is on top in the chart
	for (int i = numLines - 1; i >= 0; i--) CreateSeries(i, false);

	chart->createDefaultAxes();

	axisX = (QValueAxis*)chart->axisX();
	axisY = (QValueAxis*)chart->axisY();

	chartView = new QChartView(chart);
	ui.gridLayout->removeWidget(ui.frameChartPlaceholder);
	delete ui.frameChartPlaceholder;
	ui.gridLayout->addWidget(chartView, 0, 1, 1, 1);

	chartView->setRenderHint(QPainter::Antialiasing);

	SetXaxisText("X");
	SetYaxisText("Y");

	decNotationString = "%.2f";
	sciNotationString = "%.2e";

	notationStringX = decNotationString;
	notationStringY = decNotationString;

	minX.SetGroup(ui.editMinX, &notationStringX, axisX, true, &maxX);
	maxX.SetGroup(ui.editMaxX, &notationStringX, axisX, false, &minX);
	minY.SetGroup(ui.editMinY, &notationStringY, axisY, true, &maxY);
	maxY.SetGroup(ui.editMaxY, &notationStringY, axisY, false, &minY);

	SetXmin(0);
	SetXmax(1);
	SetYmin(0);
	SetYmax(1);

	//Set margins on the chart
	QMargins margins(0, 3, 3, 3);
	chart->setMargins(margins);
	
	chart->layout()->setContentsMargins(0,3,3,3);
	//chart->setBackgroundRoundness(0);
	chartView->setStyleSheet("background-color: white");

	//Last value and deviation frames are hidden by default
	SetHiddenLastValue(true);
	SetHiddenDeviation(true);

	//Assign the deviation and last value edits
	editLastValue.SetEdit(ui.editLastValue);
	editDeviation.SetEdit(ui.editDeviation);
	ClearDeviation();
	ClearLastValue();
	editLastValue.SetReadOnly(true);
	editDeviation.SetReadOnly(true);
}

void CwOverlay::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	
	painter.setPen(QPen(Qt::blue, 1, Qt::SolidLine, Qt::SquareCap));
	painter.drawLine(coord.x(), overlayRect.top(), coord.x(), overlayRect.bottom());
	painter.drawLine(overlayRect.left(), coord.y(), overlayRect.right(), coord.y());

	int margin = 3;

	painter.setPen(QPen(Qt::black));
	painter.setFont(QFont("Arial", 10, QFont::Bold));

	QRect yRect, xRect;
	int yAlign = 0, xAlign = 0;
	if (coord.y() > overlayRect.height() / 2)
	{
		yRect = QRect(QPoint(margin, margin), QPoint(overlayRect.right() - margin, coord.y() - margin));
		yAlign = yAlign | Qt::AlignBottom;

		xAlign |= Qt::AlignTop;
	}
	else
	{
		yRect = QRect(QPoint(margin, coord.y() + margin), QPoint(overlayRect.right() - margin, overlayRect.bottom() - margin));
		yAlign = yAlign | Qt::AlignTop;

		xAlign |= Qt::AlignBottom;
	}

	if (coord.x() > overlayRect.width() / 2)
	{
		xRect = QRect( QPoint(margin, margin), QPoint(coord.x() - margin, overlayRect.bottom() - margin) );
		xAlign = xAlign | Qt::AlignRight;

		yAlign |= Qt::AlignLeft;
	}
	else
	{
		xRect = QRect( QPoint(coord.x() + margin, margin), QPoint(overlayRect.right() - margin, overlayRect.bottom() - margin) );
		xAlign = xAlign | Qt::AlignLeft;

		yAlign |= Qt::AlignRight;
	}

	painter.drawText(xRect, xAlign, xText.c_str());
	painter.drawText(yRect, yAlign, yText.c_str());

	QWidget::paintEvent(event);
}

//Creates series from pointer and attaches it to chart
void ChartWidget::CreateSeries(int index, bool fAttachAxes /*=true*/)
{
	QLineSeries*& ser = lines[index].series;

	ser = new QLineSeries;
	QPen thePen = ser->pen();
	thePen.setCosmetic(true);
	ser->setPen(thePen);

	if (fAttachAxes)
	{
		ser->attachAxis(axisX);
		ser->attachAxis(axisY);
	}

	if (index < stdColors.Count()) ser->setColor(stdColors[index]);

	chart->addSeries(ser);
}

//create overlay showing the position of the click
void ChartWidget::CreateOverlayClick(QPoint coord)
{
	QPointF val = CoordToValue(coord);
	ShowOverlay(coord, val);
}

//create overlay showing the position of the closest data point
void ChartWidget::CreateOverlayPoint(QPoint coord)
{
	//Make sure there are lines and at least one of them has data
	if (lines.IsEmpty()) return;

	bool fDataPresent = false;
	for (auto& line : lines) if (line.Count() > 0) { fDataPresent = true; break; }
	if (!fDataPresent) return;

	//Screen coordinates of the click
	double x = coord.x();
	double y = coord.y();

	//Find minimum distance
	double minDist;
	double realMinX, realMinY;
	double screenMinX, screenMinY;
	double fInitialized = false;
	for (auto& line : lines)
	{
		for (int i = 0; i < line.Count(); i++)
		{
			double realX = line.xArray[i];
			double realY = line.yArray[i];

			//Make sure the coordinates we found are in the plot area
			if (!IsInPlotArea(realX, realY)) continue;

			//Coordinates on the point on the screen
			QPoint screenCoord = ValueToCoord(QPointF(realX, realY));
			double screenX = screenCoord.x();
			double screenY = screenCoord.y();

			//Squared distance - taking the square root is unnecessary
			//Distance between screen locations, not real distance!
			double dist = (screenX - x)*(screenX - x) + (screenY - y)*(screenY - y);

			if (!fInitialized || dist < minDist)
			{
				minDist = dist;
				realMinX = realX;
				realMinY = realY;
				screenMinX = screenX;
				screenMinY = screenY;
				fInitialized = true;
			}
		}
	}

	//If none of the points were in the plot area, return
	if (!fInitialized) return;

	//Show the overlay at minX, minY
	QPoint minCoord((int)screenMinX, (int)screenMinY);
	ShowOverlay(minCoord, QPointF(realMinX, realMinY));
}

void ChartWidget::ShowOverlay(QPoint coord, QPointF val)
{
	QPoint topLeft = ValueToCoord(QPointF(axisX->min(), axisY->max()));
	QPoint bottomRight = ValueToCoord(QPointF(axisX->max(), axisY->min()));
	QRect overlayRect(topLeft, bottomRight);

	BString xText;
	BString yText;

	xText.Format(notationStringX, val.x());
	yText.Format(notationStringY, val.y());

	overlay = new CwOverlay(chartView, overlayRect, coord, xText, yText);
	overlay->show();
}

void ChartWidget::RemoveOverlay()
{
	delete overlay;
	overlay = nullptr;
}

QPointF ChartWidget::CoordToValue(QPoint coord)
{
	QPointF scenePos = chartView->mapToScene(coord);
	QPointF chartPos = chart->mapFromScene(scenePos);
	QPointF val = chart->mapToValue(chartPos, defaultSeries);
	return val;
}

QPoint ChartWidget::ValueToCoord(QPointF val)
{
	QPointF chartPos = chart->mapToPosition(val, defaultSeries);
	QPointF scenePos = chart->mapToScene(chartPos);
	QPoint coord = chartView->mapFromScene(scenePos);
	return coord;
}

bool ChartWidget::IsInPlotArea(double x, double y)
{
	return (x >= axisX->min() && x <= axisX->max() && y >= axisY->min() && y <= axisY->max());
}

//Event filter to handle mouse clicks on the active plot area
bool ChartWidget::eventFilter(QObject *obj, QEvent *event)
{
	//Return true if event is consumed, false if it is passed on

	//Mouse release event ISN'T ROUTED TO VIEW, even if it's released over the view
	if (event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent* ev = (QMouseEvent*)event;

		if (fRightPressed && ev->button() == Qt::RightButton) 
		{
			fRightPressed = false;
			RemoveOverlay();
		}

		if (fLeftPressed && ev->button() == Qt::LeftButton)
		{
			fLeftPressed = false;
			RemoveOverlay();
		}

		return false;
	}

	//If the object isn't chartView by this point, ignore the event
	if (obj != chartView) return false;

	//If it's a click on chartView
	if (event->type() == QEvent::MouseButtonPress)
	{
		//If a button is already pressed, ignore another press
		if (fLeftPressed || fRightPressed) return false;

		QMouseEvent* ev = (QMouseEvent*)event;

		//If it's anything else other than left or right click, ignore
		if (ev->button() != Qt::RightButton && ev->button() != Qt::LeftButton) return false;
		
		//Calculate click XY position
		QPoint coord = ev->pos();
		QPointF xyPos = CoordToValue(coord);
				
		double x = xyPos.x();
		double y = xyPos.y();

		//check that the click position is within the min and max values of the axes
		if(!IsInPlotArea(x,y)) return false;

		//By now, it's a valid click!
		if (ev->button() == Qt::RightButton)
		{
			fRightPressed = true;
			CreateOverlayPoint(coord);
		}
		else if (ev->button() == Qt::LeftButton)
		{
			fLeftPressed = true;
			CreateOverlayClick(coord);
		}

		return true;
	}

	

	return QObject::eventFilter(obj, event);
}

void ChartWidget::SetSciNotation(bool val, QValueAxis* axis, BString& theNotString)
{
	BString* str;
	if (val) str = &sciNotationString;
	else str = &decNotationString;

	SetNotation(*str, axis, theNotString);
}

void ChartWidget::SetNotation(const BString& format, QValueAxis* axis, BString& theNotString)
{
	axis->setLabelFormat(QString(format));
	theNotString = format;

	UpdateEdits();
}

void ChartWidget::UpdateEdits()
{
	minX.UpdateEdit();
	maxX.UpdateEdit();
	minY.UpdateEdit();
	maxY.UpdateEdit();
}

double ChartWidget::DataLimit(bool fXaxis, bool fMin)
{
	limArray.Clear();
	for (auto& cur : lines)
	{
		if (!cur.IsEmpty()) limArray << cur.GetLimit(fXaxis, fMin);
	}

	//If there are no data lines, or all are empty, return limits of 0 (min) and 1 (max)
	if (limArray.IsEmpty())
	{
		if (fMin) return 0;
		else return 1;
	}

	//Else return the min or max out of all the data mins or maxes
	if (fMin) return limArray.Min();
	else return limArray.Max();
}