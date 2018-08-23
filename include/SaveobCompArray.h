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
#include "Saveob.h"
#include "SaveobComp.h"

//theType must be derived from SaveobComp!
//Terminating array object that casts data members to SaveobComp before all storage operations
//Storage of array of SaveobComp with full overhead
//Always creates and owns all its children

//The obInfo of the object is the same as the obInfo of its children, but with fArray=true
//When writing to string, does not store the obInfo of its children

//Calling operator= on two SaveobCompArray with different theType will cause segfault!

template <class theType> class SaveobCompArray : public Saveob
{
public:
	explicit SaveobCompArray(SaveobInfo& theInfo);
	explicit SaveobCompArray(const BString& fieldName);
	SaveobCompArray(SaveobCompArray& other);
	~SaveobCompArray(void);

private:
	CHArray<theType*> childArray;

public:
	void ResizeArrayIfSmaller(int size);
	int GetNumChildren() const {return childArray.GetNumPoints();};
	void Clear();	//Removes all children and deletes them

//Accessing the child array
	void AddChild(theType* child);	//will create a child and copy data into it
	void RemoveChild(int num);
	theType* GetChild(int num){return childArray[num];};
	theType* operator[](int num){return GetChild(num);};
	//Returns the base SaveobComp pointer for a given element in the array
	SaveobComp* GetChildBase(int num){return (SaveobComp*)childArray[num];};

//Redefining pure virtual functions
public:
	int RequiredSpace();				//Computes the number of bytes needed to store the object in a string
	void WriteToBuffer(char*& buffer);	//Writes the object and increments buffer
	void ReadFromBuffer(char*& buffer);	//Reads the object and increments buffer
	virtual void ToString(BString&){};
	virtual void FromString(const BString&){};
	void EnforceInfo();		//Ensures that object info matches the object type
	Saveob& operator=(Saveob& rhs);		//Assignment operator
	Saveob& operator=(SaveobCompArray& rhs){return operator=((Saveob&)rhs);};

private:
	void DeleteChildren();
};

//Assignment
//Does not copy object name
template <class theType> Saveob& SaveobCompArray<theType>::operator=(Saveob& rhs)
{
	if(this==&rhs) return *this;		//Avoid self-assignment
	if(!IsSameKind(rhs)) return *this;	//Do nothing with a mismatching object

	//Now we can downcast rhs
	SaveobCompArray<theType>* p=dynamic_cast<SaveobCompArray<theType>*>(&rhs);
	if(p==NULL) return *this;	//do nothing if downcast fails

	Clear();
	int numChildren=p->GetNumChildren();
	ResizeArrayIfSmaller(numChildren);

	for(int i=0;i<numChildren;i++)
	{
		AddChild(p->GetChild(i));
	}

	return *this;
}

//Ensures that the object info matches the object type
template <class theType> void SaveobCompArray<theType>::EnforceInfo()
{
	obInfo.obType=SaveobInfo::typeComp;
	obInfo.fArray=true;
}

//Constructors
template <class theType> SaveobCompArray<theType>::SaveobCompArray(SaveobInfo& theInfo)
{
	EnforceInfo();
	obInfo.SetObName(theInfo.obName);
}

template <class theType> SaveobCompArray<theType>::SaveobCompArray(const BString& fieldName)
{
	EnforceInfo();
	obInfo.SetObName(fieldName);
}

//Copy constructor
template <class theType> SaveobCompArray<theType>::SaveobCompArray(SaveobCompArray<theType>& other)
{
	EnforceInfo();
	SetObName(other);

	*this=other;
}

//Destructor
template <class theType> SaveobCompArray<theType>::~SaveobCompArray(void)
{
	DeleteChildren();
}

template<class theType> void SaveobCompArray<theType>::RemoveChild(int num)
{
	if(num<0 || num>=GetNumChildren()) return;

	if(childArray[num]) delete childArray[num];

	childArray.RemovePointAt(num);
}

template<class theType> void SaveobCompArray<theType>::Clear()
{
	DeleteChildren();
	childArray.EraseArray();
}

template<class theType> void SaveobCompArray<theType>::AddChild(theType* child)
{
	if(child)
	{
		SaveobInfo newInfo(obInfo);
		newInfo.fArray=false;

		theType* newChild=new theType(newInfo);
		*newChild=*child;
		childArray.AddAndExtend(newChild);
	}
}

template<class theType> void SaveobCompArray<theType>::ResizeArrayIfSmaller(int size)
{
	if(size>childArray.GetSize()) childArray.ResizeArrayKeepPoints(size);
}

//Cleaning up the elements
template<class theType> void SaveobCompArray<theType>::DeleteChildren()
{
	int numChildren=GetNumChildren();
	for(int i=0;i<numChildren;i++)
	{
		if(childArray[i]) delete childArray[i];
	}
}

//Required space computation
//Does not store the obInfo of its children
template<class theType> int SaveobCompArray<theType>::RequiredSpace()
{
	int result=sizeof(int);
	int numChildren=GetNumChildren();

	for(int i=0;i<numChildren;i++)
	{
		result+=GetChildBase(i)->RequiredSpace();
	}

	return result;
}

//Writing to buffer
//Does not store the obInfo of its children
template<class theType> void SaveobCompArray<theType>::WriteToBuffer(char*& buffer)
{
	int numChildren=GetNumChildren();
	WriteValToBuffer(numChildren,buffer);

	for(int i=0;i<numChildren;i++)
	{
		GetChildBase(i)->WriteToBuffer(buffer);
	}
}

//Reading from buffer
//Buffer does not store the obInfo of children
template<class theType> void SaveobCompArray<theType>::ReadFromBuffer(char*& buffer)
{
	int numChildren;
	ReadValFromBuffer(numChildren,buffer);
	
	Clear();

	childArray.ResizeIfSmaller(numChildren);
	
	SaveobInfo childInfo(obInfo);
	childInfo.fArray=false;

	for(int i=0;i<numChildren;i++)
	{
		childArray.AddPoint(new theType(obInfo));
		GetChildBase(i)->ReadFromBuffer(buffer);
	}
}