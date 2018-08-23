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

#include "SimplestXml.h"
#include "SaveobToXml.h"
#include "SaveobComp.h"
#include "SaveobTerm.h"

namespace SaveobToXml
{
	//Writing to XML
	//Writes the saveob as a single child in a document
	bool WriteSaveobToXmlFile(SaveobComp& saveob, const BString& fileName, bool fEscapeEntities = false, bool fIndented = false);
	//Adds the saveob as a child based on the name in the obInfo
	void AddSaveobToNode(SaveobComp& saveob, xml_node& node);
	//Writes the saveob data as children of the given node
	void WriteSaveobDataToNode(SaveobComp& saveob, xml_node& node);

	//Reading from XML
	//Reads the saveob from the document given the name of the saveob
	bool ReadSaveobFromXmlFile(SaveobComp& saveob, const BString& fileName);
	//Assumes that the node will have a child node with the name from the saveob
	void ReadSaveobFromNode(SaveobComp& saveob, xml_node& node);
	//Reads the saveob data from the children of the current node
	void ReadSaveobDataFromNode(SaveobComp& saveob, xml_node& node);
}