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

#include "Data.h"
#include <math.h>


CData::CData(int theSize)
{
	xArr.ResizeArray(theSize);
	yArr.ResizeArray(theSize);
}

void CData::ThinOutPoints(double maxDev)
{
	int numPoints=GetNumPoints();
	if(numPoints<3) return;

	if(maxDev<0) maxDev=abs(maxDev);

	CData interm(2);
	CData result(numPoints);

	result.AddPoint(xArr[0],yArr[0]);		//Add first

	int first=0;
	int last=2;
	while(last<numPoints)
	{
		interm.EraseData();
		interm.AddPoint(xArr[first],yArr[first]);
		interm.AddPoint(xArr[last],yArr[last]);

		bool fOut=false;
		for(int counter1=first+1;counter1<last;counter1++)
		{
			double dev=abs(interm.InterpolatePoint(xArr[counter1])-yArr[counter1]);
			if(dev>maxDev)
			{
				result.AddPoint(xArr[counter1],yArr[counter1]);
				first=counter1;
				fOut=true;
				break;
			}
			if(fOut==true) break;
		}

		if((!fOut)||(first==(last-1))) last++;
	}

	result.AddPoint(xArr[numPoints-1],yArr[numPoints-1]);

	(*this)=result;
}

CData CData::InterpolateArray(const CHArray<double>& newX)
{
	CData result(newX.GetSize());
	int newNumPoints=newX.GetNumPoints();

	for(int counter1=0;counter1<newNumPoints;counter1++)
	{
		result.AddPoint(newX[counter1],InterpolatePoint(newX[counter1]));
	}

	return result;
}

CData& CData::operator=(const CData& rhs)
{
	if(&rhs==this) return *this;					//Avoiding self-assignment

	xArr=rhs.xArr;
	yArr=rhs.yArr;

	return *this;
}

CData& CData::Concatenate(const CData& rhs)   //Parent CData must have enough space
{
	xArr.Concatenate(rhs.xArr);
	yArr.Concatenate(rhs.yArr);
	
	return *this;
}

void CData::AddPoint(double x, double y)
{
	xArr.AddAndExtend(x);
	yArr.AddAndExtend(y);
}

CData& CData::ExportPart(CData& rhs, int from, int to)
{
	if(this==&rhs) return rhs;

	xArr.ExportPart(rhs.xArr, from, to);
	yArr.ExportPart(rhs.yArr, from, to);

	return rhs;
}

void CData::EraseData()
{
	xArr.EraseArray();
	yArr.EraseArray();
}

void CData::ResizeData(int theSize)
{
	EraseData();

	xArr.ResizeArray(theSize);
	yArr.ResizeArray(theSize);
}

//Interpolate at a given x in a data where X increases
double CData::InterpolatePoint(double x)
{
	double* ub = std::upper_bound(xArr.begin(), xArr.end(), x);

	if (IsEmpty()) return 0;
	
	if (ub == xArr.begin()) return yArr.First();
	if (ub == xArr.end()) return yArr.Last();

	int pos = ub - xArr.begin();

	return yArr.arr[pos - 1] + (x - xArr.arr[pos - 1]) *
			(yArr.arr[pos] - yArr.arr[pos - 1]) / (xArr.arr[pos] - xArr.arr[pos - 1]);
}

void CData::Write(BString name) const
{
	if(!fDataPresent()) return;

	FILE* f=fopen(name,"w");
	for(int c1=0;c1<GetNumPoints();c1++)
	{
		char buffer[20];

		sprintf(buffer,"%.4e",xArr[c1]);         //x
		fputs(buffer,f);
		fputc(' ',f);

		sprintf(buffer,"%.4e",yArr[c1]);
		fputs(buffer,f);
		fputc('\n',f);
	}
	fclose(f);
}

void CData::Read(BString name)
{
	char buffer[40];
	bool x=true;
	bool eofReached=false;

	EraseData();

	FILE* f;
	if((f=fopen(name,"r"))==NULL) return;
	while(1)			
	{
		int c2=0;						//in buffer
		while(1)
		{
			int i=fgetc(f);
			if(i==EOF) {eofReached=true; break;}
			if(i<=32)
			{
				if(c2>0) break;
				else continue;
			}
			else
			{
				buffer[c2]=(char)i;
				c2++;
			}
		}
		buffer[c2]='\0';
		if(c2>0)
		{
		if(x==false) {yArr << atof(buffer); x=true;}
		else {xArr << atof(buffer); x=false;}
		}
		if(eofReached) break;
	}
	fclose(f);
}

void CData::RemoveLastPoint()
{
	if(!fDataPresent()) return;
	
	xArr.RemoveLastPoint();
	yArr.RemoveLastPoint();
}

void CData::RemoveAllPointsAfter(double x)		//for ascending data
{
	int i;
	for (i = Count() - 1; i > 0; i--) if (xArr[i]<x) break;

	xArr.RemoveAllPoinstAfter(i);
	yArr.RemoveAllPoinstAfter(i);
}

int CData::FindNearestX(double x) const
{
	if(!fDataPresent()) return -1;

	int result=0;
	for(int counter1=0;counter1<GetNumPoints();counter1++)
	{
		double curDiff=fabs(xArr[counter1]-x);

		if(curDiff<fabs(xArr[result]-x)) result=counter1;
	}

	return result;
}

double CData::Gaussian(double x, double xPos, double width,
					   double amplitude, bool fNormalizeArea) const
{
	double result;
	double ln2	=0.693147180559945309;
	double pi	=3.14159265358979324;

	result=amplitude*exp(-(x-xPos)*(x-xPos)*4*ln2/(width*width));
	
	if(fNormalizeArea)
	{
		result /= width*sqrt(pi/ln2)/2;
	}

	return result;
}

double CData::Lorentzian(double x, double xPos, double width,
						 double amplitude, bool fNormalizeArea) const
{
	double result;
	double pi	=3.14159265358979324;

	result=amplitude*(width/2)*(width/2)/((x-xPos)*(x-xPos)+(width/2)*(width/2));

	if(fNormalizeArea)
	{
		result /= width*pi/2;
	}

	return result;
}

double CData::Voigt(double x, double xPos, double gWidth, double lWidth,
					double amplitude, bool fNormalizeArea) const
{
	//////Direct integration
	double result=0;

	double calcRange		=7;		//range in gWidth
	double numCalcPoints=11+Round(calcRange*gWidth/(5*lWidth));

	double step = calcRange/(numCalcPoints-1);
	double startPos=xPos-gWidth*calcRange/2;
	double endPos=gWidth*calcRange/2+xPos;

	for(double curPos=startPos;curPos<=endPos;curPos+=step)
	{
		result+=step*
		(Gaussian(curPos-step/2,xPos,gWidth,1,true)*Lorentzian(x,curPos-step/2,lWidth,1,true)+
		Gaussian(curPos+step/2,xPos,gWidth,1,true)*Lorentzian(x,curPos+step/2,lWidth,1,true))/2;
	}

	if(!fNormalizeArea)
	{
		double max=0;		//intensity at peak

		for(double curPos=startPos;curPos<=endPos;curPos+=step)
		{
			max+=step*
				(Gaussian(curPos - step / 2, xPos, gWidth, 1, true)*Lorentzian(xPos, curPos - step / 2, lWidth, 1, true) +
				Gaussian(curPos + step / 2, xPos, gWidth, 1, true)*Lorentzian(xPos, curPos + step / 2, lWidth, 1, true)) / 2;
		}

		result*=amplitude/max;
	}

	return result;
}

double CData::Integrate() const
{
	if(!fDataPresent()) return 0;
	if(GetNumPoints()<2) return 0;

	double result=0;

	for(int counter1=0;counter1<(GetNumPoints()-1);counter1++)
	{
		result+=(yArr[counter1]+yArr[counter1+1])/2*
			(xArr[counter1+1]-xArr[counter1]);
	}

	return result;
}

double CData::IntegrateInRange(double leftLim, double rightLim) const
{
	if(!fDataPresent()) return 0;
	if(GetNumPoints()<2) return 0;

	bool inverted=false;
	double result=0;

	int leftIndex=FindNearestX(leftLim);
	int rightIndex=FindNearestX(rightLim);

	if(leftIndex==rightIndex) return 0;

	if(leftLim>rightLim) inverted=true;		//inverted

	if(leftIndex>rightIndex)		
	{
		int var;
		var=leftIndex;
		leftIndex=rightIndex;
		rightIndex=var;
	}

	for(int counter1=leftIndex;counter1<=rightIndex;counter1++)
	{
		result+=(yArr[counter1]+yArr[counter1+1])/2*
			(xArr[counter1+1]-xArr[counter1]);
	}

	if(inverted) result*=-1;
	return result;
}

void CData::LorentzianFill(double xMin,double xMax,int xNumPoints, double peakPos, double width,
						double amplitude,bool fNormalizeArea)
{
	if((xNumPoints<2)||(xMin>=xMax)) return;

	ResizeData(xNumPoints);
	double spacing=(xMax-xMin)/(xNumPoints-1);

	for(double curX=xMin;curX<=xMax;curX+=spacing)
	{
		AddPoint(curX,Lorentzian(curX,peakPos,width,amplitude,fNormalizeArea));
	}
}

void CData::GaussianFill(double xMin,double xMax,int xNumPoints, double peakPos, double width,
						double amplitude,bool fNormalizeArea)
{
	if((xNumPoints<2)||(xMin>=xMax)) return;

	ResizeData(xNumPoints);
	double spacing=(xMax-xMin)/(xNumPoints-1);

	for(double curX=xMin;curX<=xMax;curX+=spacing)
	{
		AddPoint(curX,Gaussian(curX,peakPos,width,amplitude,fNormalizeArea));
	}
}

void CData::VoigtFill(double xMin,double xMax,int xNumPoints, double peakPos, double gWidth,
						double lWidth,double amplitude,bool fNormalizeArea)
{
	if((xNumPoints<2)||(xMin>=xMax)) return;

	ResizeData(xNumPoints);
	double spacing=(xMax-xMin)/(xNumPoints-1);

	for(double curX=xMin;curX<=xMax;curX+=spacing)
	{
		AddPoint(curX,Voigt(curX,peakPos,gWidth,lWidth,amplitude,fNormalizeArea));
	}
}

int CData::Round(double val) const
{
	if((val-floor(val))>=0.5) return (int)ceil(val);
	else return (int)floor(val);
}

void CData::LinearFit(double& a, double& b)//Fit to a*x+b
{
	CHArray<double> x2(GetNumPoints()),ex(GetNumPoints());

	double s1,s2,s3,s4,s5;

	x2=xArr^2;
	ex=xArr*yArr;

	s1=x2.Sum();
	s2=xArr.Sum();
	s3=ex.Sum();
	s4=GetNumPoints();
	s5=yArr.Sum();

	double denominator=s2*s2-s1*s4;

	a=-(s3*s4-s2*s5)/denominator;
	b=(s2*s3-s1*s5)/denominator;

	return;
}

void CData::ParabolicFit(double *param)		///three parameters - a,b,c
{

	CHArray<double>	x4(GetNumPoints()), x3(GetNumPoints()), x2(GetNumPoints()), ex2(GetNumPoints()), ex(GetNumPoints());
	
	double s1,s2,s3,s4,s5,s6,s7,s8;

	x4=xArr^4;
	x3=xArr^3;
	x2=xArr^2;
	ex2=yArr*x2;
	ex=xArr*yArr;

	s1=x4.Sum();
	s2=x3.Sum();
	s3=x2.Sum();
	s4=ex2.Sum();
	s5=xArr.Sum();
	s6=ex.Sum();
	s7=(double)GetNumPoints();
	s8=yArr.Sum();

	double denominator=s3*s3*s3-2*s2*s3*s5+s1*s5*s5+s2*s2*s7-s1*s3*s7;

	param[0]=-(-s4*s5*s5+s3*s5*s6+s3*s4*s7-s2*s6*s7-s3*s3*s8+s2*s5*s8)/denominator;

	param[1]=(-s3*s4*s5+s3*s3*s6+s2*s4*s7-s1*s6*s7-s2*s3*s8+s1*s5*s8)/denominator;

	param[2]=(s3*s3*s4-s2*s4*s5-s2*s3*s6+s1*s5*s6+s2*s2*s8-s1*s3*s8)/denominator;
}

void CData::Baseline(CData &baseData)
{
	if(!fDataPresent()) return;

	CHArray<double> baseline(GetNumPoints());

	for(int counter1=0;counter1<GetNumPoints();counter1++)
	{
		baseline.AddPoint(baseData.InterpolatePoint(xArr[counter1]));
	}

	yArr-=baseline;
}

int CData::GetClosestPoint(double x, double y, double xScaling, double yScaling)
{
	int result=0;

	CHArray<double> c(GetNumPoints());

	for(int counter1=0;counter1<GetNumPoints();counter1++)
	{
		c.AddPoint(
			pow(
			(x-xArr[counter1])*(x-xArr[counter1])*xScaling*xScaling+
			(y-yArr[counter1])*(y-yArr[counter1])*yScaling*yScaling
				,0.5)
			);
	}

	result=c.PositionOfMin();

	return result;
}

void CData::NormalizeAreaTo(double area)
{
	double curArea=Integrate();

	if(curArea==0) return;
	else yArr*=(area/curArea);

	return;
}