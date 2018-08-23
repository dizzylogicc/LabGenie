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

#include "TempControllerWidget.h"
#include <algorithm>
#include "Matrix.h"

TempControllerWidget::TempControllerWidget(TempController* controllerDevice, QWidget* parent /*= 0*/) :
ExpWidget(controllerDevice, parent)
{
	ui.setupUi(this);

	SetController(controllerDevice);

	//Chart
	chart = new ChartWidget(2);
	ui.gridLayout->addWidget(chart, 1, 1);
	ui.gridLayout->removeWidget(ui.placeholderFrame);
	delete ui.placeholderFrame;

	chart->SetYaxisText("T, K");
	chart->SetXaxisText("Time, s");

	//Show the last value and deviation fields
	chart->SetHiddenLastValue(false);
	chart->SetHiddenDeviation(false);

	//Edit boxes
	editSetpoint.SetEdit(ui.editSetpoint);
	editRampRate.SetEdit(ui.editRampRate);
	editRadius.SetEdit(ui.editRadius);
	editProp.SetEdit(ui.editProp);
	editProp.SetSciFormat();
	editIntegral.SetEdit(ui.editIntegral);
	editIntegral.SetSciFormat();
	editDeriv.SetEdit(ui.editDeriv);
	editDeriv.SetSciFormat();
	editMaxV.SetEdit(ui.editMaxV);
	editMaxV.SetFormat("%.1f");
	editMaxT.SetEdit(ui.editMaxT);
	editMaxT.SetFormat("%.0f");

	OnCheckRoundedToggled(true);

	FromDevice();
}

void TempControllerWidget::SetController(TempController* theController)
{
	controller = theController;

	//Connect signals
	QObject::connect(controller, &TempController::SignalNewData, this, &TempControllerWidget::OnNewData, Qt::QueuedConnection);
	QObject::connect(controller, &TempController::SignalNewControlData, this, &TempControllerWidget::OnNewControlData, Qt::QueuedConnection);
	QObject::connect(controller, &TempController::SignalNewState, this, &TempControllerWidget::OnNewState, Qt::QueuedConnection);
}

void TempControllerWidget::ToDevice()
{
	TempControlParams params;

	params.setpoint = editSetpoint.Val();
	params.rate = editRampRate.Val();
	params.fRounded = ui.checkRounded->isChecked();

	params.radius = editRadius.Val();

	params.PIDprop = editProp.Val();
	params.PIDintegral = editIntegral.Val();
	params.PIDderiv = editDeriv.Val();

	params.maxControlV = editMaxV.Val();
	params.maxTemp = editMaxT.Val();

	params.xMin = chart->xMin();
	params.xMax = chart->xMax();
	params.yMin = chart->yMin();
	params.yMax = chart->yMax();

	controller->SetParams(params);
}

void TempControllerWidget::FromDevice()
{
	TempControlParams params = controller->Params();

	editSetpoint.SetVal(params.setpoint);
	editRampRate.SetVal(params.rate);
	ui.checkRounded->setChecked(params.fRounded);
	editRadius.SetVal(params.radius);

	editProp.SetVal(params.PIDprop);
	editIntegral.SetVal(params.PIDintegral);
	editDeriv.SetVal(params.PIDderiv);

	editMaxV.SetVal(params.maxControlV);
	editMaxT.SetVal(params.maxTemp);

	chart->SetXmax(params.xMax);
	chart->SetXmin(params.xMin);
	chart->SetYmax(params.yMax);
	chart->SetYmin(params.yMin);
}

void TempControllerWidget::OnCheckReadToggled(bool)
{
	bool fEnable = ui.checkRead->isChecked();
	if (fEnable) chart->ClearAll();

	controller->SetReading(fEnable);
}

void TempControllerWidget::OnCheckControlToggled(bool)
{
	ToDevice();
	controller->SetControlling(ui.checkControl->isChecked());
}

void TempControllerWidget::OnCheckRoundedToggled(bool)
{
	ui.editRadius->setEnabled(ui.checkRounded->isChecked());
}

void TempControllerWidget::OnSetTpressed()
{
	ToDevice();
	controller->CreateRamp();
}

void TempControllerWidget::OnSetPIDpressed()
{
	ToDevice();
}

void TempControllerWidget::OnWriteToFilePressed()
{
	CData tempData = chart->GetLineData(0);
	CData setpointData = chart->GetLineData(1);

	QString name = QFileDialog::getSaveFileName(this,
		tr("Save temperature data to file"), "",
		tr("Text files (*.txt);;All Files (*.*)"));

	BString fileName = name.toStdString();

	if (fileName == "") return;

	int numPoints = std::max(tempData.Count(), setpointData.Count());

	CMatrix<double> mat(4, numPoints);		//data is saved as a 4-column matrix
	mat = 0;	//set all to zero
	mat[0] = tempData.xArr;
	mat[1] = tempData.yArr;
	mat[2] = setpointData.xArr;
	mat[3] = setpointData.yArr;
	
	mat.Write(fileName);
}

void TempControllerWidget::OnClearDataPressed()
{
	chart->ClearAll();
	controller->ZeroStopwatchTimer();
}

void TempControllerWidget::OnSwitchOffPowerPressed()
{
	controller->SetControlling(false);
	controller->ShutDown();
}

void TempControllerWidget::OnNewData(double measured, double time)
{
	chart->Line(0).AddPoint(time, measured);
	chart->SetLastValue(measured);
}

void TempControllerWidget::OnNewControlData(double measured, double setpoint, double time)
{
	chart->Line(0).AddPoint(time, measured);
	chart->Line(1).AddPoint(time, setpoint);

	chart->SetLastValue(measured);
	chart->SetDeviation(measured - setpoint);
}

void TempControllerWidget::OnNewState(int state)
{
	if (state == pidState_standby)
	{
		ui.checkRead->setChecked(false);
		ui.checkControl->setChecked(false);

		chart->ClearDeviation();
		chart->ClearLastValue();
	}
	else if (state == pidState_reading)
	{
		ui.checkRead->setChecked(true);
		ui.checkControl->setChecked(false);

		chart->ClearDeviation();
	}
	else if (state == pidState_controlling)
	{
		ui.checkRead->setChecked(true);
		ui.checkControl->setChecked(true);
	}
}