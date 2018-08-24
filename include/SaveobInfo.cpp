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

#include "SaveobInfo.h"

SaveobInfo::SaveobInfo(void):
obName(""),
obDesig(""),
obType(typeBool),
fArray(false)
{
}

SaveobInfo::~SaveobInfo(void){}

int SaveobInfo::RequiredSpace()
{
	int result=0;
	result+=(obName.GetLength()+1);		//BString
	result+=(obDesig.GetLength()+1);	//BString
	result+=1;							//enum type obType
	result+=1;							//bool fArray

	return result;
};

//Simple serialization
void SaveobInfo::WriteToBuffer(char*& buffer)
{
	WriteValToBuffer(obName,buffer);
	WriteValToBuffer(obDesig,buffer);
	WriteValToBuffer(obType,buffer);
	WriteValToBuffer(fArray,buffer);
}

//Simple serialization
void SaveobInfo::ReadFromBuffer(char*& buffer)
{
	ReadValFromBuffer(obName,buffer);
	ReadValFromBuffer(obDesig,buffer);
	ReadValFromBuffer(obType,buffer);
	ReadValFromBuffer(fArray,buffer);
}