#pragma once
#include "Array.h"

//Cyclic array for easy addition and removal of points from both ends
//Cannot be created with a size of 0
//Allows addressing such as (*this)[-2] - second element from end, etc.
template <class theType,class intType=int>
class CyclicArray
{
public:
	CyclicArray(intType size=0):
	chArray(size,true),
	numPoints(0),
	startPos(0){};

	~CyclicArray(){};

public:
	void Resize(intType newSize)
	{
		chArray.ResizeArray(newSize,true);
		numPoints=0;
		startPos=0;
	};
	void Clear()
	{
		chArray.Clear();
		numPoints=0;
		startPos=0;
	}
	
public:
	intType Size() const {return chArray.GetSize();};
	intType Count() const {return numPoints;};
	bool isFull() const {return numPoints==Size();};
	bool isEmpty() const {return numPoints==0;};
	bool hasData() const {return numPoints>0;};
	const CHArray<theType,intType>& dataArray() const {return chArray;};

public:
	theType& operator[](intType pos)		//Position is treated as the index going from last entry in the negative direction
											//i.e., i[0] is the last element, i[1] is second from last, etc.
	{
		if(pos >= numPoints) pos = pos % numPoints;
		else {if(pos < 0) {pos = numPoints - (-pos) % numPoints;}}

		return chArray.arr[GetRealPos(startPos + numPoints -1 - pos)];
	};
	theType& First(){return (*this)[numPoints-1];};
	theType& Last(){return (*this)[0];};
	theType Average(intType numToAverage = -1);		//The average of the last numToAverage points; if -1, all available points

public:
	void Append(const theType& val);
	void Prepend(const theType& val);
	void RemoveFirst();
	void RemoveLast();
		
private:
	intType numPoints;
	intType startPos;
	CHArray<theType,intType> chArray;

private:
	intType GetRealPos(intType pos);
};

template <class theType,class intType>
void CyclicArray<theType,intType>::Append(const theType& val)
{
	intType addPos = GetRealPos(startPos + numPoints);
	chArray.arr[addPos]=val;

	if(numPoints < Size()) numPoints++;
	else startPos=GetRealPos(startPos+1);
}

template <class theType,class intType>
void CyclicArray<theType,intType>::Prepend(const theType& val)
{
	intType addPos = GetRealPos(startPos - 1);
	chArray.arr[addPos]=val;

	if(numPoints < Size()) numPoints++;
	startPos = addPos;
}

template <class theType,class intType>
intType CyclicArray<theType,intType>::GetRealPos(intType pos)
{
	if(Size()==0) return 0;

	if(pos >= Size()) pos = pos % Size();
	else {if(pos < 0) {pos = Size() - ((-pos) % Size());}}

	return pos;
}

template <class theType,class intType>
void CyclicArray<theType,intType>::RemoveFirst()
{
	if(numPoints==0) return;
	startPos=GetRealPos(startPos+1);
	numPoints--;
}

template <class theType,class intType>
void CyclicArray<theType,intType>::RemoveLast()
{
	if(numPoints==0) return;
	numPoints--;
}

template <class theType,class intType>
theType CyclicArray<theType,intType>::Average(intType numToAverage)		//The average of the last numToAverage points
{
	if(numToAverage > numPoints || numToAverage == -1) numToAverage = numPoints;

	theType result = (theType)0;
	for(intType i = 0; i<numToAverage; i++) result += (*this)[i];

	return result/numToAverage;
}