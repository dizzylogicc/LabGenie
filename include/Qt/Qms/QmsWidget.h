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

#include "Qt/ExpWidget.h"
#include "ui_QmsWidget.h"

#include "Array.h"
#include "Qt/ChartWidget/ChartWidget.h"
#include "Qt/Qms/QmsDataPoint.h"
#include "Qt/Qms/ExpDeviceQmsHidenHAL.h"
#include <QtCharts>
#include "Qt/Qms/ExpDeviceQmsHidenHAL.h"

#define maxQmsDatasets 10

class QmsWidget : public ExpWidget
{
	Q_OBJECT

public:
	QmsWidget(ExpDeviceQms* theQms, QWidget* parent = 0);
	~QmsWidget(){}

public slots:
	void OnCheckAutorangingToggled() { HandleEnabling(); }
	void OnComboModeChanged() { HandleEnabling(); }
	void OnComboDetectorChanged() { HandleEnabling(); }
	void OnBnConnectClicked();
	void OnBnStartClicked();
	void OnBnSendClicked();
	void OnBnTurnOnClicked();
	void OnBnTurnOffClicked();
	void OnBnStopNowClicked();
	void OnBnWriteToFileClicked();

	//Handling signals from the mass spec
	void OnNewData(CHArray<QmsDataPoint> newPoints);		//New data points coming from the mass spec
	void OnNewCom(BString str);								//New communication between the mass spec and the computer
	void OnDatasetStart();									//New dataset is starting on the mass spec
	void OnSpecStart();										//New spec is starting on the mass spec

	void OnNewPowerState() { HandleEnabling(); }
	void OnNewConnState() { HandleEnabling(); }
	void OnNewScanState() { HandleEnabling(); }

protected:
	void PopulateRangeCombo(QComboBox* combo);
	void HandleEnabling();		//Enables and disables widgets according to the current state

	void FromDevice();			//Loads settings to the interface from the device
	void ToDevice();			//Saves settings from the interface to the device

	void SetYChartLimits();
	void SetTimeAxisLimits();

	int HidenRangeToMenuIndex(int range);
	int MenuIndexToHidenRange(int menuIndex);

	void MassTableToDevice();
	void MassTableFromDevice();
	void ClearMassTable();
	void SetMassTableColors();

protected:
	//State queries
	bool IsSpectrumMode() const;								//Whether we are in spectrum mode or not
	bool IsAutorangingOn() const;
	bool IsDetectorSEM() const;									//Whether the detector is in SEM mode or Faraday

protected:
	//Arrays of widgets for disabling and enabling according to current state
	CHArray<QWidget*> widgetsMode;
	CHArray<QWidget*> widgetsDetector;
	CHArray<QWidget*> widgetsAcquisition;
	CHArray<QWidget*> widgetsSpectrum;
	CHArray<QWidget*> widgetsSigTime;
	CHArray<QWidget*> widgetsAutoranging;
	CHArray<QWidget*> widgetsDetectorSEM;
	CHArray<QWidget*> widgetsAllQmsSettings;
	CHArray<QWidget*> widgetsConnection;
	CHArray<QWidget*> widgetsSerialComm;
	CHArray<QWidget*> widgetsScanControl;
	CHArray<QWidget*> widgetsPowerButtons;
	CHArray<QWidget*> widgetsAll;

	BString comString;

protected:
	ExpDeviceQms& qms;
	ChartWidget* chart;
	CHArray<double> massTableCopy;		//Copying the mass table for use in the signal-time mode acquisition

private:
	Ui::QmsWidgetClass ui;
};