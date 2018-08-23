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
#include "BString.h"

class Saveob;		//Forward declaration to avoid circular dependency

class SaveobInfo
{
public:
	SaveobInfo(void);
	SaveobInfo(const BString& theObName, const BString& theType, bool theFarray)
	{
		obName=theObName;
		fArray=theFarray;

		if(theType=="bool") {obType=typeBool;return;}
		if(theType=="char") {obType=typeChar;return;}
		if(theType=="int") {obType=typeInt;return;}
		if(theType=="uint") {obType=typeUint;return;}
		if(theType=="int64") {obType=typeInt64;return;}
		if(theType=="float") {obType=typeFloat;return;}
		if(theType=="double") {obType=typeDouble;return;}
		if(theType=="BString") {obType=typeBString;return;}
		if(theType=="comp") {obType=typeComp;return;}
	}

	~SaveobInfo(void);

public:
	enum type:char
	{
		typeBool=0,			//Terminating objects
		typeChar=1,
		typeInt=2,
		typeUint=3,
		typeInt64=4,
		typeFloat=5,
		typeDouble=6,
		typeBString=7,
		typeComp=8			//SaveobComp or SaveobCompArray
	};

public:
	int RequiredSpace();
	void WriteToBuffer(char*& buffer);		//Increments buffer after the operation
	void ReadFromBuffer(char*& buffer);		//Increments buffer after the operation

public:
	void SetObName(const BString& name){obName=name;};
	void SetObName(const SaveobInfo& otherInfo){obName=otherInfo.obName;};
	void SetObDesig(const BString& desig){obDesig=desig;};
	//Checks whether the object is of the same kind - obType and fArray
	bool IsSameKind(const SaveobInfo& other){return ((obType==other.obType) && (fArray==other.fArray));};

public:
	template <class theType>  void WriteValToBuffer(theType& val, char*& buffer);		//Writes and increments buffer
	template <class theType>  void ReadValFromBuffer(theType& val, char*& buffer);		//Reads and increments buffer

//Comparison
	bool operator==(const SaveobInfo& rhs)
	{
		return ((obName==rhs.obName) && (obType==rhs.obType) && (fArray==rhs.fArray));
	};

//Object data, saved in serialization
public:
	BString obName;		//Name - a label attached to an object, for example "creationDate"
	BString obDesig;		//Designation - specifies class of an object, e.g. "date". Can be empty.
	type obType;			//the type of the object (see type enum above)
	bool fArray;			//whether it is an array
};

template<class theType>
void SaveobInfo::WriteValToBuffer(theType& val,char*& buffer)
{
	*((theType*)buffer)=val;
	buffer+=sizeof(theType);
}

template<class theType>
void SaveobInfo::ReadValFromBuffer(theType& val,char*& buffer)
{
	val=*((theType*)buffer);
	buffer+=sizeof(theType);
}

template<> inline void SaveobInfo::WriteValToBuffer(BString& val,char*& buffer)
{
	char* theString=(char*)((const char*)val);
	while(*theString!=0) {*buffer=*theString;buffer++;theString++;}
	*buffer=0;buffer++;
}

template<> inline void SaveobInfo::ReadValFromBuffer(BString& val,char*& buffer)
{
	val=buffer;
	buffer+=(val.GetLength()+1);
}