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
#include "Saveob.h"
#include "Array.h"

//Terminating array savable object
//Serializes an array of simple types to/from string
//No default constructors
template <class theType> class SaveobTermArray : public Saveob
{
public:
	explicit SaveobTermArray(SaveobInfo& theInfo);
	SaveobTermArray(SaveobInfo& theInfo, CHArray<theType>& theTarget);
	SaveobTermArray(const BString& fieldName, CHArray<theType>& theTarget);
	SaveobTermArray(SaveobTermArray& other);	//Copy constructor
	~SaveobTermArray(void);

private:
	//Common function for all EnforceInfo specializations
	void EnforceInfoCommon();

private:
	//Conversions of between scalar values and BString
	BString ValToString(const theType& val);
	theType StringToVal(const BString& str);

public:
	CHArray<theType>* target;
	bool fOwnsTarget;

public:
	int RequiredSpace();					//Computes the number of bytes needed to store the object in a string
	void WriteToBuffer(char*& buffer);		//Writes the object and increments buffer
	void ReadFromBuffer(char*& buffer);		//Reads the object and increments buffer
	void EnforceInfo();				//Ensures that obInfo matches the object type - fully specialized

	virtual void ToStringArray(CHArray<BString>& result);
	virtual void FromStringArray(const CHArray<BString>& source);

	Saveob& operator=(Saveob& rhs);	//Assignment operator
	Saveob& operator=(SaveobTermArray& rhs){return operator=((Saveob&)rhs)};
};

//Assignment operator
template <class theType> Saveob& SaveobTermArray<theType>::operator=(Saveob& rhs)
{
	if(this==&rhs) return *this;	//avoid self-assignment
	if(!IsSameKind(rhs)) return *this;	//not the same type of object, do nothing
	
	//Now we can downcast rhs
	SaveobTermArray<theType>* p=dynamic_cast<SaveobTermArray<theType>*>(&rhs);
	if(p==NULL) return *this;	//do nothing if downcast fails

	*target=*(p->target);
	return *this;
}

//Constructors
template<class theType> SaveobTermArray<theType>::SaveobTermArray(SaveobInfo& theInfo)
{
	EnforceInfo();
	obInfo.SetObName(theInfo.obName);

	target=new CHArray<theType>;
	fOwnsTarget=true;
}

template<class theType> SaveobTermArray<theType>::SaveobTermArray(SaveobInfo& theInfo, CHArray<theType>& theTarget)
{
	EnforceInfo();
	obInfo.SetObName(theInfo.obName);

	target=&theTarget;
	fOwnsTarget=false;
}

template<class theType> SaveobTermArray<theType>::
	SaveobTermArray(const BString& fieldName, CHArray<theType>& theTarget)
{
	EnforceInfo();
	obInfo.SetObName(fieldName);

	target=&theTarget;
	fOwnsTarget=false;
}

//Copy constructor
template<class theType> SaveobTermArray<theType>::
	SaveobTermArray(SaveobTermArray<theType>& other)
{
	EnforceInfo();
	SetObName(other);

	target=new CHArray<theType>;
	fOwnsTarget=true;

	*this=other;
}

//Destructor
template<class theType> SaveobTermArray<theType>::~SaveobTermArray()
{
	if(fOwnsTarget) delete target;
}

//Required space computation
template<class theType> int SaveobTermArray<theType>::RequiredSpace()	//Arrays of simple types and BString
{return target->RequiredSpace();}	
/////End required space computation

//Writing to buffer
template<class theType> void SaveobTermArray<theType>::WriteToBuffer(char*& buffer)		//Arrays of simple types and BString
{target->SerializeToBuffer(buffer);}
//End writing to buffer

//Reading from buffer
template<class theType> void SaveobTermArray<theType>::ReadFromBuffer(char*& buffer)	//Arrays of simple types and BString
{target->SerializeFromBuffer(buffer);}
//End reading from buffer

//Enforcing correct info object - specialized functions
template <class theType> void SaveobTermArray<theType>::EnforceInfoCommon()
{obInfo.fArray=true;}

#define ENFORCE_INFO_ARR(type,enumType) template<> inline void SaveobTermArray<type>::EnforceInfo(){EnforceInfoCommon();obInfo.obType=enumType;}

ENFORCE_INFO_ARR(bool, SaveobInfo::typeBool)
ENFORCE_INFO_ARR(char, SaveobInfo::typeChar)
ENFORCE_INFO_ARR(int, SaveobInfo::typeInt)
ENFORCE_INFO_ARR(uint, SaveobInfo::typeUint)
ENFORCE_INFO_ARR(int64, SaveobInfo::typeInt64)
ENFORCE_INFO_ARR(float, SaveobInfo::typeFloat)
ENFORCE_INFO_ARR(double, SaveobInfo::typeDouble)
ENFORCE_INFO_ARR(BString, SaveobInfo::typeBString)

//Conversions of scalar value to string
template<> inline BString SaveobTermArray<bool>::ValToString(const bool& val) { if (val) return "true"; else return "false"; }	//bool
template<class theType> inline BString SaveobTermArray<theType>::ValToString(const theType& val)
								{BString result; result.Format("%i", val); return result;}				//char, int, uint, int64
template<> inline BString SaveobTermArray<float>::ValToString(const float& val) 
								{BString result; result.Format("%.5e", val); return result;}			//float
template<> inline BString SaveobTermArray<double>::ValToString(const double& val)
								{BString result; result.Format("%.5e", val); return result;}			//double
template<> inline BString SaveobTermArray<BString>::ValToString(const BString& val) { return val; }		//BString

//Conversions of string to scalar value
template<> inline bool SaveobTermArray<bool>::StringToVal(const BString& str) { if (str == "true") return true; else return false; }	//bool
template<class theType> inline theType SaveobTermArray<theType>::StringToVal(const BString& str) { return atoi(str); }			//char, int, uint, int64
template<> inline float SaveobTermArray<float>::StringToVal(const BString& str) { return (float)atof(str); }					//float
template<> inline double SaveobTermArray<double>::StringToVal(const BString& str) { return (double)atof(str); }					//double
template<> inline BString SaveobTermArray<BString>::StringToVal(const BString& str) { return str; }								//BString


template<class theType> void SaveobTermArray<theType>::ToStringArray(CHArray<BString>& result)
{
	result.ResizeIfSmaller(target->Count());
	result.Clear();

	for (auto& x : *target) result.AddPoint(ValToString(x));
}

template<class theType> void SaveobTermArray<theType>::FromStringArray(const CHArray<BString>& source)
{
	target->ResizeIfSmaller(source.Count());
	target->Clear();

	for (auto& x : source) target->AddPoint(StringToVal(x));
}
