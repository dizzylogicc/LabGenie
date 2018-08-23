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

#include "SaveobToXml.h"
#include "Common.h"

//Writes the saveob as a single child in a document
bool SaveobToXml::WriteSaveobToXmlFile(SaveobComp& saveob, const BString& fileName, bool fEscapeEntities, bool fIndented)
{
	xml_document doc;
	AddSaveobToNode(saveob, doc);
	return SimplestXml::WriteNodeToFile(fileName, doc, fEscapeEntities, fIndented);
}

//Adds the saveob as a child based on the name in the obInfo
//If a child with the same name already exists, then clear that child and re-use it
void SaveobToXml::AddSaveobToNode(SaveobComp& saveob, xml_node& node)
{
	xml_node curNode = node.child(saveob.obInfo.obName);
	if(!node) curNode = node.append_child(saveob.obInfo.obName);
	WriteSaveobDataToNode(saveob, curNode);
}

//Writes the saveob data as children of the given node
void SaveobToXml::WriteSaveobDataToNode(SaveobComp& saveob, xml_node& node)
{
	CHArray<Saveob*> children;
	saveob.GetChildren(children);

	for (int i = 0; i<children.Count(); i++)
	{
		Saveob& curChild = *(children[i]);

		//If it's a comp child, create a subnode for it with a recursive call
		if (curChild.obInfo.obType == SaveobInfo::typeComp)
		{
			AddSaveobToNode((SaveobComp&)curChild, node);
			continue;
		}

		//Find the node with the same name and clear it, or attach one if it does not exist
		xml_node mainNode = node.child(curChild.obInfo.obName);
		if (mainNode) SimplestXml::RemoveAllChildren(mainNode);
		else mainNode = node.append_child(curChild.obInfo.obName);

		//If it is an array, save its elements into <val> child nodes
		if (curChild.obInfo.fArray)
		{
			CHArray<BString> tempArray;
			curChild.ToStringArray(tempArray);

			for (auto& curString : tempArray)
				mainNode.append_child("val").append_child(pugi::node_pcdata).set_value(curString);

			continue;
		}

		//By now it's a data saveob and not an array
		//Append it as a node with a pcdata
		BString curString;
		curChild.ToString(curString);
		mainNode.append_child(pugi::node_pcdata).set_value(curString);
	}
}

//Reads the saveob from the document given the name of the saveob
bool SaveobToXml::ReadSaveobFromXmlFile(SaveobComp& saveob, const BString& fileName)
{
	xml_document doc;
	if(!SimplestXml::ReadNodeFromFile(fileName, doc)) return false;

	ReadSaveobFromNode(saveob, doc);
	return true;
}

//Assumes that the node will have a child node with the name from the saveob
void SaveobToXml::ReadSaveobFromNode(SaveobComp& saveob, xml_node& node)
{
	xml_node curNode = node.child(saveob.obInfo.obName);
	if(!curNode) return;

	ReadSaveobDataFromNode(saveob, curNode);
	
}

void SaveobToXml::ReadSaveobDataFromNode(SaveobComp& saveob, xml_node& node)
{
	CHArray<Saveob*> children;
	saveob.GetChildren(children);

	for (int i = 0; i<children.Count(); i++)
	{
		Saveob& curChild = *(children[i]);

		//Try to find the node for this child
		xml_node childNode = node.child(curChild.obInfo.obName);
		if (!childNode) continue;

		//If it's a comp child, read it via a recursive call
		if (curChild.obInfo.obType == SaveobInfo::typeComp)
		{
			ReadSaveobFromNode((SaveobComp&)curChild, node);
			continue;
		}

		//If it's an array, read its elements from the <val> child nodes in childNode
		if (curChild.obInfo.fArray)
		{
			int count = 0;
			xml_node curVal;

			//Count the number of elements
			for (curVal = childNode.child("val"); curVal; curVal = curVal.next_sibling("val")) count++;

			//Store the strings into a temp array
			CHArray<BString> tempArray(count);
			for (curVal = childNode.child("val"); curVal; curVal = curVal.next_sibling("val"))
			{
				xml_node pcDataNode = curVal.first_child();
				if (!pcDataNode || pcDataNode.type() != pugi::node_pcdata) continue;

				tempArray << pcDataNode.value();
			}

			curChild.FromStringArray(tempArray);
			continue;
		}

		//By now it's a data saveob and not an array
		//Read into it from the pcdata
		xml_node pcDataNode = childNode.first_child();
		if (!pcDataNode || pcDataNode.type() != pugi::node_pcdata) continue;

		BString curString = pcDataNode.value();
		curChild.FromString(curString);
	}
}
