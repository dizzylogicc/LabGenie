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

#include "SaveobComp.h"
#include "Common.h"

//Assignment operator
//Does not copy object name
Saveob& SaveobComp::operator=(Saveob& rhs)
{
	if(this==&rhs) return *this;		//Avoid self-assignment
	if(!IsSameKind(rhs)) return *this;	//Do nothing with a mismatching object

	//Downcasting rhs
	SaveobComp* p=dynamic_cast<SaveobComp*>(&rhs);
	if(p==NULL) return *this;	//do nothing if downcast fails

	//Remove fields not present in rhs
	auto iter=childMap.begin();
	while(iter!=childMap.end())
	{
		Saveob* child=iter->second.child;
		if(child==NULL) {iter++;continue;}	//something's wrong

		if(p->IsChildPresent(child->obInfo.obName))	//such name is present in rhs
		{iter++;continue;}
		else	//no such name in rhs - delete child in *this
		{
			iter++;
			DeleteChild(child->obInfo.obName);
			continue;
		}
	}

	//Copy children one by one
	for(auto rhsIter=p->childMap.begin();rhsIter!=p->childMap.end();rhsIter++)
	{
		Saveob* rhsChild=rhsIter->second.child;
		if(rhsChild==NULL) continue;	//something's wrong

		BString& rhsName=rhsChild->GetObName();
		Saveob* child=GetChild(rhsName);

		if(child==NULL)	//No such child in the current object, add it and copy data
		{
			AddChildAndOwn(rhsChild->obInfo);
			child=GetChild(rhsName);
		}
		else	//check that the child type matches
		{
			//type does not match, delete child and create a new one
			if(!child->IsSameKind(*rhsChild))
			{
				DeleteChild(rhsName);
				AddChildAndOwn(rhsChild->obInfo);
				child=GetChild(rhsName);
			}
		}
		if(child==NULL) continue;	//something's wrong

		(*child)=(*rhsChild);	//now we can call assignment
	}

	//Copy obId
	obId=p->obId;

	return *this;
}

//Ensuring that the info matches the object type
void SaveobComp::EnforceInfo()
{
	obInfo.obType=SaveobInfo::typeComp;
	obInfo.fArray=false;
}

//Destructor - delete the children that the object owns
SaveobComp::~SaveobComp(void)
{
	DeleteOwnedChildren();
};

void SaveobComp::DeleteOwnedChildren()
{
	for(auto iter=childMap.begin();iter!=childMap.end();iter++)
	{
		ChildElement& element=iter->second;
		if(element.fOwnsChild && element.child) delete element.child;
	}
}

//Constructors
SaveobComp::SaveobComp(const BString& fieldName)
{
	EnforceInfo();
	obInfo.SetObName(fieldName);
}

SaveobComp::SaveobComp(SaveobInfo& theInfo)
{
	EnforceInfo();
	obInfo.SetObName(theInfo.obName);
}

//Copy constructor
SaveobComp::SaveobComp(SaveobComp& other)
{
	EnforceInfo();
	SetObName(other);

	*this=other;
}

//Child access
Saveob* SaveobComp::GetChild(const BString& childName)
{
	auto iter=childMap.find(childName);
	if(iter==childMap.end()) return NULL;

	return iter->second.child;
}

//Export pointers to all the children into the provided array
void SaveobComp::GetChildren(CHArray<Saveob*>& childArr)
{
	childArr.Clear();

	for(auto iter=childMap.begin();iter!=childMap.end();iter++)
	{childArr.AddAndExtend((iter->second).child);}
}

bool SaveobComp::InternalAddChild(Saveob* child, bool fOwned)
{
	if(!child) return false;
	if(childMap.count(child->obInfo.obName)>0) return false;	//Such child already exists

	ChildElement element;
	element.child=child;
	element.fOwnsChild=fOwned;

	childMap[child->obInfo.obName]=element;
	return true;
}

//will create child and own it, if such name does not exist
bool SaveobComp::AddChildAndOwn(SaveobInfo& info)
{
	if(childMap.count(info.obName)>0) return false;

	Saveob* newOb=CreateCorrectObject(info);
	if(!newOb) return false;

	ChildElement element;
	element.child=newOb;
	element.fOwnsChild=true;

	childMap[info.obName]=element;
	return true;
}

int SaveobComp::RequiredSpace()
{
	//Objects never save their own obInfo, just the obInfos of their children
	int result=0;
	result+=sizeof(int);	//to save the number of children
	
	for(auto iter=childMap.begin();iter!=childMap.end();iter++)
	{
		ChildElement& curElement=iter->second;
		result+=curElement.child->obInfo.RequiredSpace();
		result+=curElement.child->RequiredSpace();
	}

	return result;
}

void SaveobComp::WriteToBuffer(char*& buffer)
{
	int numChildren=(int)childMap.size();

	//Save number of children
	WriteValToBuffer(numChildren,buffer);

	//Save obInfo and the child itself for each child
	for(auto iter=childMap.begin();iter!=childMap.end();iter++)
	{
		ChildElement& curElement=iter->second;
		curElement.child->obInfo.WriteToBuffer(buffer);
		curElement.child->WriteToBuffer(buffer);
	}
}

//Read from buffer will not delete unused fields already present
void SaveobComp::ReadFromBuffer(char*& buffer)
{
	//Read number of children
	int numChildren;
	ReadValFromBuffer(numChildren,buffer);

	//Read each child's obInfo and the child itself
	for(int i=0;i<numChildren;i++)
	{
		SaveobInfo curInfo;
		curInfo.ReadFromBuffer(buffer);

		//Find out whether the object with this name is in this saveobcomp
		auto iter=childMap.find(curInfo.obName);
		if(iter==childMap.end())	//No such object - add it first
		{
			AddChildAndOwn(curInfo);
			iter=childMap.find(curInfo.obName);
		}
		else	//Such name exists
		{
			if(!(iter->second.child->obInfo == curInfo))	//ObInfo does not match! Delete the current child and create a new one
			{
				DeleteChild(curInfo.obName);
				AddChildAndOwn(curInfo);
				iter=childMap.find(curInfo.obName);
			}
		}

		//Read from buffer
		iter->second.child->ReadFromBuffer(buffer);
	}
}

void SaveobComp::DeleteChild(const BString& name)
{
	auto iter=childMap.find(name);
	if(iter==childMap.end()) return; //no such element

	ChildElement& curElement=iter->second;
	if(curElement.fOwnsChild && curElement.child) {delete curElement.child;}

	childMap.erase(iter);
}

//Remove all children
void SaveobComp::Clear()
{
	DeleteOwnedChildren();
	childMap.clear();
}
