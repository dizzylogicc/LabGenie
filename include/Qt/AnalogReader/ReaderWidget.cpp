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

#include "ReaderWidget.h"

ReaderWidget::ReaderWidget(AnalogReader* readerDevice, QWidget *parent)
	: ExpWidget(readerDevice, parent)
{
	ui.setupUi(this);

	SetReader(readerDevice);

	//Chart
	chart = new ChartWidget(2);
	ui.verticalLayout->addWidget(chart);
	ui.verticalLayout->removeWidget(ui.placeholderFrame);
	delete ui.placeholderFrame;

	chart->SetYaxisText("Value");
	chart->SetXaxisText("Time, s");

	//Show the last value and deviation fields
	chart->SetHiddenLastValue(false);

	FromDevice();
}

void ReaderWidget::SetReader(AnalogReader* theReader)
{
	reader = theReader;

	//Connect signals
	QObject::connect(reader, &AnalogReader::SignalNewData, this, &ReaderWidget::OnNewData, Qt::QueuedConnection);
}

void ReaderWidget::ToDevice()
{
	reader->xMin = chart->xMin();
	reader->xMax = chart->xMax();
	reader->yMin = chart->yMin();
	reader->yMax = chart->yMax();
}

void ReaderWidget::FromDevice()
{
	
	chart->SetXmax(reader->xMax);
	chart->SetXmin(reader->xMin);
	chart->SetYmax(reader->yMax);
	chart->SetYmin(reader->yMin);
}

void ReaderWidget::OnCheckReadToggled()
{
	bool fEnable = ui.checkRead->isChecked();
	if (fEnable)
	{
		chart->ClearAll();
		reader->StartContinuous();
		timer.SetTimerZero(0);
	}
	else reader->StopContinuous();
}

void ReaderWidget::OnWriteToFilePressed()
{
	QString name = QFileDialog::getSaveFileName(this,
		tr("Save data to file"), "",
		tr("Text files (*.txt);;All Files (*.*)"));

	BString fileName = name.toStdString();

	if (fileName == "") return;

	CData data = chart->GetLineData(0);
	data.Write(fileName);
}

void ReaderWidget::OnClearDataPressed()
{
	chart->ClearAll();
	timer.SetTimerZero(0);
}

void ReaderWidget::OnNewData(double measured)
{
	double curTime = timer.GetCurTime(0);
	chart->Line(0).AddPoint(curTime, measured);
	chart->SetLastValue(measured);
}
