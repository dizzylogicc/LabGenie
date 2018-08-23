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

#include "Saveob.h"

#include "SaveobComp.h"
#include "SaveobCompArray.h"

//Creates and returns the object of the correct type
Saveob* Saveob::CreateCorrectObject(SaveobInfo& info)
{
	if(info.fArray)	//it is an array object
	{
		if(info.obType==SaveobInfo::typeComp) return new SaveobCompArray<SaveobComp>(info);	//array of composite objects
		if(info.obType==SaveobInfo::typeBool) return new SaveobTermArray<bool>(info);
		if(info.obType==SaveobInfo::typeChar) return new SaveobTermArray<char>(info);
		if(info.obType==SaveobInfo::typeInt) return new SaveobTermArray<int>(info);
		if(info.obType==SaveobInfo::typeUint) return new SaveobTermArray<uint>(info);
		if(info.obType==SaveobInfo::typeInt64) return new SaveobTermArray<int64>(info);
		if(info.obType==SaveobInfo::typeFloat) return new SaveobTermArray<float>(info);
		if(info.obType==SaveobInfo::typeDouble) return new SaveobTermArray<double>(info);
		if(info.obType==SaveobInfo::typeBString) return new SaveobTermArray<BString>(info);
	}
	else	//it is a scalar object
	{
		if(info.obType==SaveobInfo::typeComp) return new SaveobComp(info);			//composite object
		if(info.obType==SaveobInfo::typeBool) return new SaveobTerm<bool>(info);
		if(info.obType==SaveobInfo::typeChar) return new SaveobTerm<char>(info);
		if(info.obType==SaveobInfo::typeInt) return new SaveobTerm<int>(info);
		if(info.obType==SaveobInfo::typeUint) return new SaveobTerm<uint>(info);
		if(info.obType==SaveobInfo::typeInt64) return new SaveobTerm<int64>(info);
		if(info.obType==SaveobInfo::typeFloat) return new SaveobTerm<float>(info);
		if(info.obType==SaveobInfo::typeDouble) return new SaveobTerm<double>(info);
		if(info.obType==SaveobInfo::typeBString) return new SaveobTerm<BString>(info);
	}

	return NULL;
}

