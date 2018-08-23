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
#include <map>

#include "SaveobTerm.h"
#include "SaveobTermArray.h"

//Composite savable object
//The only type of object that can be saved directly to the database
//All other savable object must be children of SaveobComp
//No default constructors!
class SaveobComp : public Saveob
{
	//Will keep a map to child elements
	class ChildElement
	{
	public:
		ChildElement():child(NULL),fOwnsChild(false){}
		Saveob* child;
		bool fOwnsChild;
	};

public:
	explicit SaveobComp(const BString& fieldName);
	explicit SaveobComp(SaveobInfo& theInfo);
	SaveobComp(SaveobComp& other);	//copy constructor
	virtual ~SaveobComp(void);

//Redefining pure virtual functions
public:
	int RequiredSpace();					//Computes the number of bytes needed to store the object in a string
	void WriteToBuffer(char*& buffer);		//Writes the object and increments buffer
	void ReadFromBuffer(char*& buffer);		//Reads the object and increments buffer
	void EnforceInfo();		//Ensures that the info object matches the object type
	Saveob& operator=(Saveob& rhs);	//Assignment operator
	Saveob& operator=(SaveobComp& rhs){return operator=((Saveob&)rhs);}

//Object data
private:
	std::map<BString,ChildElement> childMap;
public:
	int64 obId;

//Access to the child array
public:
	void GetChildren(CHArray<Saveob*>& childArr);	//Export pointers to all the children into the provided array
	int GetNumChildren(){return (int)childMap.size();}
	bool IsChildPresent(const BString& childName){return (childMap.count(childName)>0);}
	Saveob* GetChild(const BString& childName);
	Saveob* operator[](const BString& childName){return GetChild(childName);}
	void DeleteChild(const BString& childName);
	void Clear();	//Remove all children

public:
	//Adding children
	bool AddChild(Saveob* child){return InternalAddChild(child,false);}		//will not own the child; returns if this name already exists
	bool AddChildAndOwn(SaveobInfo& info);					//will create child and own it, if such name does not exist; child will own the target
	bool AddChildAndOwn(Saveob* child){return InternalAddChild(child,true);}	//Will add external child and own it

	//templated AddChildAndOwn functions, will create a child and own it, but will not own the base data item
	template <class theType> bool AddChildAndOwn(const BString& fieldName, theType& target);	//adding a scalar
	template <class theType> bool AddChildAndOwn(const BString& fieldName, CHArray<theType>& target);	//adding an array

private:
	bool InternalAddChild(Saveob* child, bool fOwned);
	void DeleteOwnedChildren();
};

//Add a scalar variable as a child
template <class theType> inline bool SaveobComp::AddChildAndOwn(const BString& fieldName, theType& target)
{
	SaveobTerm<theType>* newChild=new SaveobTerm<theType>(fieldName, target);

	if(AddChildAndOwn(newChild)) return true;
	else
	{
		delete newChild;
		return false;
	}
}

//Add an array variable as a child
template <class theType> inline bool SaveobComp::AddChildAndOwn(const BString& fieldName, CHArray<theType>& target)
{
	SaveobTermArray<theType>* newChild=new SaveobTermArray<theType>(fieldName, target);

	if(AddChildAndOwn(newChild)) return true;
	else
	{
		delete newChild;
		return false;
	}
}

