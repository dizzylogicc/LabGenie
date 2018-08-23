#pragma once
#include "Array.h"
#include "Savable.h"

template <class theType, class intType=int> class CArrArr : public Savable
{
public:
	//Constructor from numbers of points
	CArrArr(intType numArrays=0, intType arrSize=0, bool fSetMaxPoints=false):
	arr(numArrays,true)
	{Resize(numArrays,arrSize,fSetMaxPoints);};

	//Create a virtual array of arrays
	CArrArr(const CArrArr<theType,intType>& otherArrArr, intType startNum, intType numToInclude):
	arr(otherArrArr.arr.arr + startNum, numToInclude, true){};

	explicit CArrArr(const BString& fileName){Load(fileName);};
	~CArrArr(void){};

//Container behavior
public:
	CHArray<theType, intType>* begin(){ return arr.arr; }
	CHArray<theType, intType>* end(){ return arr.arr + arr.Count(); }

//The only data member
public:
	CHArray<CHArray<theType,intType>,intType> arr;

public:
	CHArray<theType,intType>& operator[](intType num) const {return arr[num];};
	void Resize(intType numArrays, intType arrSize, bool fSetMaxPoints=false);
	void Serialize(BArchive& archive);
};

template <class theType, class intType>
void CArrArr<theType,intType>::Resize(intType numArrays, intType arrSize, bool fSetMaxPoints)
{
	arr.ResizeArray(numArrays,true);

	for(intType counter1=0;counter1<numArrays;counter1++)
	{
		arr[counter1].ResizeArray(arrSize,fSetMaxPoints);
	}
}

template <class theType, class intType>
void CArrArr<theType,intType>::Serialize(BArchive& archive)
{
	intType numArrays;

	if(archive.IsStoring())
	{
		numArrays=arr.GetNumPoints();
		archive<<numArrays;
	}
	else
	{
		archive>>numArrays;
		Resize(numArrays,0,false);
	}

	for(intType counter1=0;counter1<numArrays;counter1++)
	{
		arr[counter1].Serialize(archive);
	}
}
