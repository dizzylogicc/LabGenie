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
#include "SaveobInfo.h"
#include "Array.h"

//A mapped, serializable, database-savable object
//Base class (abstract)
class Saveob
{
public:
	Saveob(void){};
	virtual ~Saveob(void){};

//Pure virtual functions - must be redefined in the final classes
public:
	virtual int RequiredSpace()=0;					//Computes the number of bytes needed to store the object in a buffer
	virtual void WriteToBuffer(char*& buffer)=0;	//Writes the object and shifts the buffer pointer
	virtual void ReadFromBuffer(char*& buffer)=0;	//Reads the object and shifts the buffer pointer
	virtual void EnforceInfo()=0;					//Ensures that ObInfo corresponds to the type of the object
	virtual Saveob& operator=(Saveob& rhs)=0;		//assignment operator

//Writing and reading from strings
//Only SaveobTerm and SaveobTerm class implement these functions
public:
	virtual void ToString(BString&){}
	virtual void FromString(const BString&){}
	virtual void ToStringArray(CHArray<BString>&){}
	virtual void FromStringArray(const CHArray<BString>&){}

//Convenience functions
	bool IsSameKind(const Saveob& other){return obInfo.IsSameKind(other.obInfo);}
	BString GetObName() const {return obInfo.obName;}
	BString ObName() const { return GetObName(); }
	void SetObName(const BString& name){obInfo.SetObName(name);}
	void SetObName(const SaveobInfo& info){obInfo.SetObName(info);}
	void SetObName(const Saveob& other){obInfo.SetObName(other.obInfo.obName);}

//Object data
public:
	SaveobInfo obInfo;	//Object never saves its own obInfo, just the obInfos of its children

//Convenience functions utilizing the methods of SaveobInfo
public:
	template <class theType>  void WriteValToBuffer(theType& val, char*& buffer)	//Writes the value and shifts the buffer pointer
	{obInfo.WriteValToBuffer(val,buffer);}
	template <class theType>  void ReadValFromBuffer(theType& val, char*& buffer)	//Reads the value and shifts the buffer pointer
	{obInfo.ReadValFromBuffer(val,buffer);}

//Creating an object from SaveobInfo
public:
	Saveob* CreateCorrectObject(SaveobInfo& info);	//creates and returns the correct type of Saveob based on the info passed to it
};
