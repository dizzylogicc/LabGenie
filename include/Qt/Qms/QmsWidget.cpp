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

#include "QmsWidget.h"
#include "QtUtils.h"
#include "SaveobToXml.h"
#include "Matrix.h"
#include <algorithm>

Q_DECLARE_METATYPE(BString)
Q_DECLARE_METATYPE(CHArray<QmsDataPoint>)

//using namespace QtCharts;

QmsWidget::QmsWidget(ExpDeviceQms* theQms, QWidget *parent) : 
	ExpWidget(theQms, parent),
	qms(*theQms)
{
	ui.setupUi(this);

	//Get rid of buttons on the spin boxes
	ui.spinSemVoltage->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinHidenDwell->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinHidenSettleTime->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinFilCurrent->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinSpecStart->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinSpecEnd->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinSpecStep->setButtonSymbols(QAbstractSpinBox::NoButtons);
	ui.spinFilament->setButtonSymbols(QAbstractSpinBox::NoButtons);

	//Set mode combobox
	ui.comboMode->addItem("Mass spectrum");
	ui.comboMode->addItem("Signal vs time");

	//Set detector combobox
	ui.comboDetector->addItem("SEM");
	ui.comboDetector->addItem("Faraday cup");

	//Set num scans combobox
	ui.comboNumScans->addItem("Single scan");
	ui.comboNumScans->addItem("Keep scanning");

	//Populate widget lists for enabling/disabling
	widgetsMode << ui.comboMode;
	widgetsDetector << ui.comboDetector << ui.spinFilament << ui.spinSemVoltage << ui.spinFilCurrent;
	widgetsSpectrum << ui.spinSpecStart << ui.spinSpecEnd << ui.spinSpecStep << ui.comboNumScans;
	widgetsSigTime << ui.tableMasses;
	widgetsAcquisition << ui.comboHidenRange << ui.checkHidenAutoranging << ui.comboHidenRangeMin
		<< ui.comboHidenRangeMax << ui.spinHidenDwell << ui.spinHidenSettleTime;
	widgetsAllQmsSettings << widgetsMode << widgetsDetector << widgetsAcquisition << widgetsSpectrum << widgetsSigTime;
	widgetsAutoranging << ui.comboHidenRangeMax << ui.comboHidenRangeMin;
	widgetsDetectorSEM << ui.spinSemVoltage;
	widgetsConnection << ui.bnConnect << ui.comboPorts;
	widgetsSerialComm << ui.textEditCom << ui.editCommand << ui.bnSend;
	widgetsScanControl << ui.bnStartScan << ui.bnStopScanNow << ui.bnStopScanWhenDone;
	widgetsPowerButtons << ui.bnTurnOn << ui.bnTurnOff;

	widgetsAll << widgetsAllQmsSettings << widgetsConnection << widgetsSerialComm <<
		widgetsScanControl << widgetsPowerButtons;

	//Set the available ports in the ports combo
	qms.EnumeratePorts();
	for (int i = 0; i < qms.portList.Count(); i++)
	{
		ui.comboPorts->addItem((qms.portList[i] + " - " + qms.portDescList[i]).c_str());
	}

	//Create the chart
	chart = new ChartWidget(maxQmsDatasets);
	ui.verticalLayoutChart->removeWidget(ui.frameChartPlaceholder);
	delete ui.frameChartPlaceholder;
	ui.verticalLayoutChart->insertWidget(1, chart);
	chart->SetSciNotationY(true);

	//Populate range comboboxes
	PopulateRangeCombo(ui.comboHidenRange);
	PopulateRangeCombo(ui.comboHidenRangeMin);
	PopulateRangeCombo(ui.comboHidenRangeMax);

	//Set the mass table
	QHeaderView *verticalHeader = ui.tableMasses->verticalHeader();
	verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
	verticalHeader->setDefaultSectionSize(21);

	QHeaderView *horHeader = ui.tableMasses->horizontalHeader();
	horHeader->setSectionResizeMode(QHeaderView::Fixed);
	horHeader->setDefaultSectionSize(96);

	ui.tableMasses->setColumnCount(1);
	ui.tableMasses->setRowCount(maxQmsDatasets);

	ClearMassTable();

	//Connect the signals from the mass spec
	qRegisterMetaType<BString>();
	qRegisterMetaType<CHArray<QmsDataPoint>>();
	QObject::connect(&qms, &ExpDeviceQms::SignalNewData, this, &QmsWidget::OnNewData, Qt::QueuedConnection);
	QObject::connect(&qms, &ExpDeviceQms::SignalNewCom, this, &QmsWidget::OnNewCom, Qt::QueuedConnection);
	QObject::connect(&qms, &ExpDeviceQms::SignalDatasetStart, this, &QmsWidget::OnDatasetStart, Qt::QueuedConnection);
	QObject::connect(&qms, &ExpDeviceQms::SignalSpecStart, this, &QmsWidget::OnSpecStart, Qt::QueuedConnection);
	QObject::connect(&qms, &ExpDeviceQms::SignalNewPowerState, this, &QmsWidget::OnNewPowerState, Qt::QueuedConnection);
	QObject::connect(&qms, &ExpDeviceQms::SignalNewScanState, this, &QmsWidget::OnNewScanState, Qt::QueuedConnection);
	QObject::connect(&qms, &ExpDeviceQms::SignalNewConnState, this, &QmsWidget::OnNewConnState, Qt::QueuedConnection);

	FromDevice();

	//Enable and disable widgets
	HandleEnabling();
}

void QmsWidget::PopulateRangeCombo(QComboBox* combo)
{
	for (int i = 5; i < 11; i++)
	{
		QString text;
		text.sprintf("10^-%i Torr", i);
		combo->addItem(text);
	}
}

void QmsWidget::HandleEnabling()
{
	//Scanning - disable everything except for the stop buttons
	using QtUtils::EnableWidgets;
	using QtUtils::SetWidgetsVisible;

	//Handle visibility
	ui.groupBoxSpectrum->setVisible(IsSpectrumMode());
	ui.groupBoxSigTime->setVisible(!IsSpectrumMode());

	//Enable all
	EnableWidgets(widgetsAll, true);

	//If not connected, disable power buttons, serial comms and scan control
	//If connected, disable connection controls
	if (!qms.IsConnected())
	{
		EnableWidgets(widgetsPowerButtons, false);
		EnableWidgets(widgetsSerialComm, false);
		EnableWidgets(widgetsScanControl, false);
	}
	else
	{
		EnableWidgets(widgetsConnection, false);
	}

	//Disable power buttons and detector control depending on the power state
	//Disable scan control when power is off
	if (qms.IsPowerOn())
	{
		ui.bnTurnOn->setEnabled(false);
		EnableWidgets(widgetsDetector, false);
	}
	else
	{
		EnableWidgets(widgetsScanControl, false);
		ui.bnTurnOff->setEnabled(false);
	}

	//If scanning, disable power buttons, serial comms, all settings, button start
	//If not scanning, disable stop buttons and selectively disable some settings
	if (qms.IsScanning())
	{
		EnableWidgets(widgetsPowerButtons, false);
		EnableWidgets(widgetsSerialComm, false);
		EnableWidgets(widgetsAllQmsSettings, false);
		ui.bnStartScan->setEnabled(false);
		if (qms.IsStopNowRequested()) ui.bnStopScanNow->setEnabled(false);
		if (qms.IsStopNowRequested() || qms.IsStopAtEndRequested()) ui.bnStopScanWhenDone->setEnabled(false);
	}
	else
	{
		ui.bnStopScanNow->setEnabled(false);
		ui.bnStopScanWhenDone->setEnabled(false);
		if (!IsAutorangingOn()) EnableWidgets(widgetsAutoranging, false);
		if (!IsDetectorSEM()) EnableWidgets(widgetsDetectorSEM, false);
	}
}

//Whether we are in spectrum mode or not
bool QmsWidget::IsSpectrumMode() const
{
	return ui.comboMode->currentIndex() == 0;
}


bool QmsWidget::IsAutorangingOn() const
{
	return ui.checkHidenAutoranging->isChecked();
}

bool QmsWidget::IsDetectorSEM() const
{
	return ui.comboDetector->currentIndex() == 0;
}

//User requests a connection to qms through a given port
void QmsWidget::OnBnConnectClicked()
{
	if (qms.portList.IsEmpty())
	{
		QtUtils::ErrorBox("Unable to connect: no ports are available.");
		return;
	}

	BString portName = qms.portList[ui.comboPorts->currentIndex()];
	qms.SetQmsPort(portName);

	bool fConnected = qms.Connect();

	BString statusString;
	if (fConnected)
	{
		BString idString = qms.GetIdString();
		statusString = "Connected: " + idString;

	}
	else statusString = "Connection failed.";

	ui.labelConnectStatus->setText(statusString.c_str());
}

void QmsWidget::ToDevice()
{
	qms.detectorType = ui.comboDetector->currentIndex();
	qms.fHidenAutoranging = ui.checkHidenAutoranging->isChecked();
	qms.emission = ui.spinFilCurrent->value();
	qms.filament = ui.spinFilament->value();
	qms.hidenDwell = ui.spinHidenDwell->value();
	qms.hidenMaxRange = MenuIndexToHidenRange(ui.comboHidenRangeMax->currentIndex());
	qms.hidenMinRange = MenuIndexToHidenRange(ui.comboHidenRangeMin->currentIndex());
	qms.hidenCurRange = MenuIndexToHidenRange(ui.comboHidenRange->currentIndex());
	qms.hidenSettleTime = ui.spinHidenSettleTime->value();

	MassTableToDevice();

	qms.opMode = ui.comboMode->currentIndex();
	if (ui.comboPorts->count() > 0)
	{
		int index = ui.comboPorts->currentIndex();
		if (index < qms.portList.Count() && index > 0) qms.portString = qms.portList[index];
	}

	qms.semVoltage = ui.spinSemVoltage->value();
	qms.specEnd = ui.spinSpecEnd->value();
	qms.specMode = ui.comboNumScans->currentIndex();
	qms.specStart = ui.spinSpecStart->value();
	qms.specStep = ui.spinSpecStep->value();
}

void QmsWidget::FromDevice()
{
	ui.comboDetector->setCurrentIndex(qms.detectorType);
	ui.checkHidenAutoranging->setChecked(qms.fHidenAutoranging);
	ui.spinFilCurrent->setValue(qms.emission);
	ui.spinFilament->setValue(qms.filament);
	ui.spinHidenDwell->setValue(qms.hidenDwell);
	ui.comboHidenRangeMax->setCurrentIndex(HidenRangeToMenuIndex(qms.hidenMaxRange));
	ui.comboHidenRangeMin->setCurrentIndex(HidenRangeToMenuIndex(qms.hidenMinRange));
	ui.comboHidenRange->setCurrentIndex(HidenRangeToMenuIndex(qms.hidenCurRange));
	ui.spinHidenSettleTime->setValue(qms.hidenSettleTime);
	ui.comboMode->setCurrentIndex(qms.opMode);
	ui.spinSemVoltage->setValue(qms.semVoltage);
	ui.spinSpecEnd->setValue(qms.specEnd);
	ui.comboNumScans->setCurrentIndex(qms.specMode);
	ui.spinSpecStart->setValue(qms.specStart);
	ui.spinSpecStep->setValue(qms.specStep);

	//Handle mass table
	MassTableFromDevice();

	//Load serial port string
	int portIndex = qms.portList.PositionOf(qms.portString);
	if (portIndex != -1 && portIndex < ui.comboPorts->count()) ui.comboPorts->setCurrentIndex(portIndex);
}

int QmsWidget::HidenRangeToMenuIndex(int range)
{
	int result = -range - 5;
	if (result < 0) result = 0;
	if (result > 5) result = 5;
	return result;
}

int QmsWidget::MenuIndexToHidenRange(int menuIndex)
{
	int result = -menuIndex - 5;
	if (result < -10) result = -10;
	if (result > -5) result = -5;
	return result;
}

void QmsWidget::MassTableToDevice()
{
	qms.massTable.Clear();

	for (int i = 0; i < ui.tableMasses->rowCount(); i++)
	{
		BString text = ui.tableMasses->item(i, 0)->text().toStdString();
		double val = atof(text);
		if (val <= 0) continue;
		qms.massTable << val;
	}
}

void QmsWidget::MassTableFromDevice()
{
	ClearMassTable();

	for (int i = 0; i < qms.massTable.Count() && i < ui.tableMasses->rowCount(); i++)
	{
		BString str;
		str.Format("%.2f", qms.massTable[i]);
		ui.tableMasses->item(i, 0)->setText(str.c_str());
	}
}

void QmsWidget::ClearMassTable()
{
	for (int i = 0; i < ui.tableMasses->rowCount(); i++) ui.tableMasses->setItem(i, 0, new QTableWidgetItem(""));

	SetMassTableColors();
}

void QmsWidget::SetMassTableColors()
{
	if (!chart) return;

	CHArray<QColor> lineColors = chart->GetLineColors();
	for (int i = 0; i < lineColors.Count(); i++)
	{
		QTableWidgetItem* curItem = ui.tableMasses->item(i, 0);
		curItem->setBackgroundColor(lineColors[i]);
		curItem->setTextColor(Qt::white);
		curItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	}
}

void QmsWidget::OnNewCom(BString str)
{
	comString = str + "\n\n" + comString;
	if (comString.GetLength() > 20000) comString = comString.Left(20000);

	ui.textEditCom->setPlainText(comString.c_str());
}

void QmsWidget::OnBnSendClicked()
{
	BString command = ui.editCommand->text().toStdString();
	qms.SendReceive(command);

	ui.editCommand->setText("");
}

void QmsWidget::OnBnTurnOnClicked()
{
	ToDevice();
	qms.PowerOn();
}

void QmsWidget::OnBnTurnOffClicked()
{
	qms.PowerOff();
}

void QmsWidget::OnBnStartClicked()
{
	ToDevice();
	qms.Start();

	massTableCopy = qms.massTable;
}

//New dataset is starting, clear the plot
void QmsWidget::OnDatasetStart()
{
	chart->ClearAll();

	chart->SetYaxisText("Signal");

	if (qms.opMode == opMode_spectrum)
	{
		chart->SetXmin(qms.specStart);
		chart->SetXmax(qms.specEnd);
		chart->SetXaxisText("m/e, a.m.u.");
	}
	else if (qms.opMode == opMode_sigTime)
	{
		chart->SetXmin(0);
		chart->SetXmax(20);
		chart->SetXaxisText("Time, s");
	}
}

void QmsWidget::OnSpecStart()		//A new spectrum is starting
{
	chart->Line(1).CopyFromLine(chart->Line(0));
	chart->Line(0).Clear();
	SetYChartLimits();
}

void QmsWidget::OnNewData(CHArray<QmsDataPoint> newPoints)
{
	if (qms.opMode == opMode_spectrum)
	{
		for (auto& point : newPoints) chart->Line(0).AddPoint(point.mass, point.signal);
	}
	else if(qms.opMode == opMode_sigTime)
	{
		for (auto& point : newPoints)
		{
			//Find correct index in the mass table
			int index = massTableCopy.PositionOfClosest(point.mass);
			chart->Line(index).AddPoint(point.time, point.signal);
		}

		SetTimeAxisLimits();
	}

	SetYChartLimits();
}

void QmsWidget::SetYChartLimits()
{
	chart->SetYmin(chart->DataYmin());
	chart->SetYmax(chart->DataYmax());
}

void QmsWidget::SetTimeAxisLimits()
{
	double maxTime = chart->DataXmax();

	if (maxTime > chart->xMax()) chart->SetXmax(maxTime * 1.618);		//Golden ratio increase : ))
}

void QmsWidget::OnBnStopNowClicked()
{
	qms.StopNow();
}

void QmsWidget::OnBnWriteToFileClicked()
{
	CMatrix<double> mat;		//Matrix into which all data will be written

	if (qms.opMode == opMode_spectrum)
	{
		//Is there a second spectrum? If so, get data from it. Else, take data from the first spectrum.
		CData data;
		if (chart->LineCount() > 1 && chart->Line(1).Count() > 0) data = chart->GetLineData(1);
		else if (chart->LineCount() > 0 && chart->Line(0).Count() > 0) data = chart->GetLineData(0);
		else
		{
			QtUtils::ErrorBox("Error: no data to save.");
			return;
		}

		mat.ResizeMatrix(2, data.Count());
		mat[0] = data.xArr;
		mat[1] = data.yArr;
	}
	else if (qms.opMode == opMode_sigTime)
	{
		//How many masses do we have?
		int numMasses = massTableCopy.Count();

		if (numMasses == 0)
		{
			QtUtils::ErrorBox("Error: no masses are specified in the mass table.");
			return;
		}
		
		//Find the highest number of rows that we need
		int numRows = numMasses;
		for (int i = 0; i < numMasses; i++)
		{
			int curCount = chart->Line(i).Count();
			if (numRows < curCount) numRows = curCount;
		}

		mat.ResizeMatrix(numMasses * 2 + 1, numRows);
		mat = 0;

		//Copy the data to matrix
		mat[0] = massTableCopy;		//masses are saved in the first column
		for (int i = 0; i < numMasses; i++)
		{
			CData data = chart->GetLineData(i);
			mat[1 + i * 2] = data.xArr;
			mat[1 + i * 2 + 1] = data.yArr;
		}
	}
	
	QString name = QFileDialog::getSaveFileName(this,
		tr("Save data to file"), "",
		tr("Text files (*.txt);;All Files (*.*)"));

	BString fileName = name.toStdString();
	if (fileName == "") return;
	mat.Write(fileName);
}