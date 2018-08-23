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

#include "Array.h"

class CData  
{
public:
	CData(int theSize=100);
	virtual ~CData(){};

public:
	CHArray<double> xArr;
	CHArray<double> yArr;

public:
	int Size() const {return xArr.GetSize();}
	int GetNumPoints() const {return xArr.GetNumPoints();}
	int Count() const { return GetNumPoints(); }
	bool IsEmpty() const { return Count() == 0; }
	bool fDataPresent() const {return (GetNumPoints()!=0);}
	bool fArrayFull() const {return xArr.IsFull();}

public:
	void AddTrapezium(CData& data, double totArea=0)
					{AddTrapezium(data.xArr[0],data.yArr[0],data.xArr[1],data.yArr[1],totArea);}
	void AddTrapezium(double x1, double y1, double x2, double y2, double totArea=0);
																	//Right-agle trapezium added to data
																	//X is viewed as boundaries for bins
																	//Only for equispaced X

	CData InterpolateArray(const CHArray<double>& newX);			//Returns interpolated CData
	int GetClosestPoint(double x, double y, double xScaling, double yScaling);
	void Baseline(CData& baseData);
	void ParabolicFit(double* param);
	void LinearFit(double& a, double& b);
	
	double Voigt(double x, double xPos, double gWidth,
					double lWidth, double amplitude, bool fNormalizeArea) const;
	double Lorentzian(double x, double xPos, double width,
					double amplitude,bool fNormalizeArea) const;
	double Gaussian(double x, double xPos, double width,
					double amplitude, bool fNormalizeArea) const;

	void LorentzianFill(double xMin,double xMax,int xNumPoints, double peakPos, double width,
						double amplitude,bool fNormalizeArea);
	void GaussianFill(double xMin,double xMax,int xNumPoints, double peakPos, double width,
						double amplitude,bool fNormalizeArea);
	void VoigtFill(double xMin,double xMax,int xNumPoints, double peakPos, double gWidth,
						double lWidth, double amplitude,bool fNormalizeArea);

	double Integrate() const;
	double IntegrateInRange(double leftLim, double rightLim) const;
	void NormalizeAreaTo(double area);
	int FindNearestX(double x) const;
	void RemoveAllPointsAfter(double x);
	void RemoveLastPoint();
	void Read(BString name);
	void Write(BString name) const;
	double InterpolatePoint(double x);
	void ResizeData(int theSize);
	void Resize(int theSize) { ResizeData(theSize); }

	void ResizeIfSmaller(int theSize)
	{
		if (Size() < theSize) Resize(theSize);
		Clear();
	}

	void EraseData();
	void Clear() { EraseData(); }
	void AddPoint(double x, double y);
	int Round(double val) const;
	void ThinOutPoints(double maxDev);		//Removes points on linear regions
											//Maximum deviation below maxDev

	CData& Concatenate(const CData& rhs);   //Parent CData should have enough space
	CData& ExportPart(CData& rhs, int from, int to);
	CData& operator=(const CData& rhs);
};