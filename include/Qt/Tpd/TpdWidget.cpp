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

#include "TpdWidget.h"
#include <QLayout>
#include "Data.h"

Q_DECLARE_METATYPE(TpdChartPoint)
Q_DECLARE_METATYPE(CData)

TpdWidget::TpdWidget(ExpDeviceTpd* theTpdDevice, QWidget* parent) :
devTpd(theTpdDevice),
ExpWidget(theTpdDevice, parent)
{
	ui.setupUi(this);

	ui.spinExpNumber->setButtonSymbols(QAbstractSpinBox::NoButtons);

	//Add the chart for tpd
	tpdChart = new ChartWidget(10,this);
	ui.horLayoutTpd->removeWidget(ui.frameTpdChart);
	delete ui.frameTpdChart;
	ui.horLayoutTpd->insertWidget(1,tpdChart);

	tpdChart->SetXaxisText("Temperature, K");
	tpdChart->SetYaxisText("Signal");
	tpdChart->SetSciNotationY(true);

	//Add temp controller widget
	tcWidget = new TempControllerWidget(devTpd->TempControlDev(), this);
	QVBoxLayout* tempLayout = new QVBoxLayout;
	tempLayout->addWidget(tcWidget);
	ui.groupBoxTempControl->setLayout(tempLayout);

	//Set the edits
	editFrom.SetEdit(ui.editFrom);
	editFrom.SetFormat("%.1f");
	editTo.SetEdit(ui.editTo);
	editTo.SetFormat("%.1f");
	editRate.SetEdit(ui.editRate);
	editRate.SetFormat("%.3f");
	editDelay.SetEdit(ui.editDelay);
	editDelay.SetFormat("%.1f");
	editDuration.SetEdit(ui.editDuration);
	editDuration.SetFormat("%.1f");

	//Connect signals
	qRegisterMetaType<TpdChartPoint>();
	qRegisterMetaType<CData>();
	QObject::connect(devTpd, &ExpDeviceTpd::SignalNewState, this, &TpdWidget::OnNewState, Qt::QueuedConnection);
	QObject::connect(devTpd, &ExpDeviceTpd::SignalNewTpdData, this, &TpdWidget::OnNewTpdData, Qt::QueuedConnection);
	QObject::connect(devTpd, &ExpDeviceTpd::SignalResetLineData, this, &TpdWidget::OnResetLineData, Qt::QueuedConnection);
	QObject::connect(devTpd, &ExpDeviceTpd::SignalDataWritten, this, &TpdWidget::OnDataWritten, Qt::QueuedConnection);

	FromDevice();
	CheckIsothermalChanged(0);
	OnNewState(devTpd->State());
}

void TpdWidget::ToDevice()
{
	devTpd->tempFrom = editFrom.Val();
	devTpd->tempTo = editTo.Val();
	devTpd->rate = editRate.Val();
	devTpd->delay = editDelay.Val();
	devTpd->fIsothermal = ui.checkIsothermal->isChecked();
	devTpd->duration = editDuration.Val();
	devTpd->expNumber = ui.spinExpNumber->value();
}

void TpdWidget::FromDevice()
{
	editFrom.SetVal(devTpd->tempFrom);
	editTo.SetVal(devTpd->tempTo);
	editRate.SetVal(devTpd->rate);
	editDelay.SetVal(devTpd->delay);
	ui.checkIsothermal->setChecked(devTpd->fIsothermal);
	editDuration.SetVal(devTpd->duration);
	ShowFolder();
}

void TpdWidget::ShowFolder()
{
	int maxLength = 29;
	BString str = devTpd->folder;

	if (str.GetLength() > maxLength) str = "..." + str.Right(maxLength);

	ui.editFolder->setText(str.c_str());
}

void TpdWidget::CheckIsothermalChanged(int)
{
	bool fChecked = ui.checkIsothermal->isChecked();
	
	editTo.SetEnabled(!fChecked);
	editDelay.SetEnabled(!fChecked);
	editDuration.SetEnabled(fChecked);
}

void TpdWidget::OnNewState(int state)
{
	if (state == tpdState_idle)
	{
		ui.bnStartTpd->setEnabled(true);
		ui.bnStopTpd->setEnabled(false);
		ui.bnSelectFolder->setEnabled(true);
		ui.labelStatus->setText("TPD: idle");
	}
	else if (state == tpdState_running)
	{
		ui.bnStartTpd->setEnabled(false);
		ui.bnStopTpd->setEnabled(true);
		ui.bnSelectFolder->setEnabled(false);
		ui.labelStatus->setText("TPD: running");
	}
	else if (state == tpdState_finished)
	{
		ui.bnStartTpd->setEnabled(true);
		ui.bnStopTpd->setEnabled(false);
		ui.bnSelectFolder->setEnabled(true);
		ui.labelStatus->setText("TPD: finished");
	}
}

void TpdWidget::OnNewTpdData(TpdChartPoint point)
{
	tpdChart->Line(point.index).AddPoint(point.tempOrTime, point.signal);

	tpdChart->SetYmin(tpdChart->DataYmin());
	tpdChart->SetYmax(tpdChart->DataYmax());
}

void TpdWidget::OnStartTpdClicked()
{
	ToDevice();
	devTpd->StartTpd();

	if (devTpd->State() != tpdState_running) return;

	tpdChart->ClearAll();

	if (devTpd->fIsothermal)
	{
		tpdChart->SetXmin(0);
		tpdChart->SetXmax(devTpd->TpdDuration());
		tpdChart->SetXaxisText("Time, s");
	}
	else
	{
		tpdChart->SetXmin(devTpd->tempFrom);
		tpdChart->SetXmax(devTpd->tempTo);
		tpdChart->SetXaxisText("Temperature, K");
	}
}

void TpdWidget::OnStopTpdClicked()
{
	devTpd->StopTpd();
}

void TpdWidget::OnResetLineData(CData data, int index)
{
	tpdChart->SetLineData(data, index);
}

void TpdWidget::OnSelectFolderClicked()
{
	BString dir = QFileDialog::getExistingDirectory(this, tr("Select directory where TPD data will be saved"),
		devTpd->folder.c_str(),
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks).toStdString();

	if (dir == "") return;

	devTpd->folder = dir;
	ShowFolder();
}

//The device has written the data
//Increment experiment #
void TpdWidget::OnDataWritten()
{
	ui.spinExpNumber->setValue(ui.spinExpNumber->value() + 1);
}