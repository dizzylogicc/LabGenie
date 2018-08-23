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

//Terminating savable object - templated
//No default constructors
template <class theType> class SaveobTerm : public Saveob
{
public:
	explicit SaveobTerm(SaveobInfo& theInfo);
	SaveobTerm(SaveobInfo& theInfo, theType& theTarget);
	SaveobTerm(const BString& fieldName, theType& theTarget);
	SaveobTerm(SaveobTerm& other);	//Copy-constructor
	~SaveobTerm(void);

private:
	//Common function for all EnforceInfo specializations
	void EnforceInfoCommon();

public:
	theType* target;
	bool fOwnsTarget;

//Redefining pure virtual functions
public:
	int RequiredSpace();					//Computes the number of bytes needed to store the object in a string
	void WriteToBuffer(char*& buffer);		//Writes the object and increments buffer
	void ReadFromBuffer(char*& buffer);		//Reads the object and increments buffer

	void ToString(BString& str);
	void FromString(const BString& str);

	void EnforceInfo();				//Ensures that the object info matches the object type
	Saveob& operator=(Saveob& rhs);	//redefined assignment operator
	Saveob& operator=(SaveobTerm& rhs){return operator=((Saveob&)rhs);}
};

//Assignment operator
template <class theType> Saveob& SaveobTerm<theType>::operator=(Saveob& rhs)
{
	if(this==&rhs) return *this;	//avoid self-assignment
	if(!IsSameKind(rhs)) return *this;	//not the same type of object, do nothing
	
	//Now we can downcast rhs
	SaveobTerm<theType>* p=dynamic_cast<SaveobTerm<theType>*>(&rhs);
	if(p==NULL) return *this;	//Do nothing if downcast fails

	*target=*(p->target);		//copy target data
	return *this;
}

//Constructors
template <class theType> SaveobTerm<theType>::SaveobTerm(SaveobInfo& theInfo)
{
	EnforceInfo();
	obInfo.SetObName(theInfo.obName);

	target=new theType;
	fOwnsTarget=true;
}

template <class theType> SaveobTerm<theType>::SaveobTerm(SaveobInfo& theInfo, theType& theTarget)
{
	EnforceInfo();
	obInfo.SetObName(theInfo.obName);

	target=&theTarget;
	fOwnsTarget=false;
}

template <class theType> SaveobTerm<theType>::SaveobTerm(const BString& fieldName, theType& theTarget)
{
	EnforceInfo();
	obInfo.SetObName(fieldName);

	target=&theTarget;
	fOwnsTarget=false;
}

//Copy constructor
template <class theType> SaveobTerm<theType>::SaveobTerm(SaveobTerm<theType>& other)
{
	EnforceInfo();
	SetObName(other);

	target=new theType;
	fOwnsTarget=true;

	*this=other;
}

//Destructor
template <class theType> SaveobTerm<theType>::~SaveobTerm(void)
{
	if(fOwnsTarget) delete target;
}

//Required space computation
template <class theType> int SaveobTerm<theType>::RequiredSpace(){return sizeof(theType);}	//all simple types
template<> inline int SaveobTerm<BString>::RequiredSpace(){return target->GetLength()+1;}			//BString
/////End required space computation

//Writing to buffer
template <class theType> void SaveobTerm<theType>::WriteToBuffer(char*& buffer)		//All simple types and BString
{WriteValToBuffer(*target,buffer);}
//End writing to buffer

//Reading from buffer
template <class theType> void SaveobTerm<theType>::ReadFromBuffer(char*& buffer)	//All simple types and BString
{ReadValFromBuffer(*target,buffer);}
//End reading from buffer

//Enforcing correct info structure
template <class theType> void SaveobTerm<theType>::EnforceInfoCommon()
{obInfo.fArray=false;}

//Conversions to string
template<> inline void SaveobTerm<bool>::ToString(BString& str){ if (*target) str = "true";	else str = "false";} //bool
template<class theType> inline void SaveobTerm<theType>::ToString(BString& str) {str.Format("%i",*target);}		//char, int, uint, int64
template<> inline void SaveobTerm<float>::ToString(BString& str) {str.Format("%.5e",*target);}					//float
template<> inline void SaveobTerm<double>::ToString(BString& str) {str.Format("%.5e",*target);}					//double
template<> inline void SaveobTerm<BString>::ToString(BString& str) {str = *target;}								//BString

//Conversions from string
template<> inline void SaveobTerm<bool>::FromString(const BString& str){if (str == "true") (*target) = true; else (*target) = false;} //bool
template<class theType> inline void SaveobTerm<theType>::FromString(const BString& str) {*target = atoi(str);}				//char, int, uint, int64
template<> inline void SaveobTerm<float>::FromString(const BString& str) {*target = (float)atof(str);}						//float
template<> inline void SaveobTerm<double>::FromString(const BString& str) {*target = (double)atof(str);}					//double
template<> inline void SaveobTerm<BString>::FromString(const BString& str) {*target = str;}									//BString

//Type enforcement
#define ENFORCE_INFO(type, enumType) template<> inline void SaveobTerm<type>::EnforceInfo() {EnforceInfoCommon();obInfo.obType=enumType;}

ENFORCE_INFO(bool, SaveobInfo::typeBool)
ENFORCE_INFO(char, SaveobInfo::typeChar)
ENFORCE_INFO(int, SaveobInfo::typeInt)
ENFORCE_INFO(uint, SaveobInfo::typeUint)
ENFORCE_INFO(int64, SaveobInfo::typeInt64)
ENFORCE_INFO(float, SaveobInfo::typeFloat)
ENFORCE_INFO(double, SaveobInfo::typeDouble)
ENFORCE_INFO(BString, SaveobInfo::typeBString)


