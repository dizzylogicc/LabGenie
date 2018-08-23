#pragma once
#include "Array.h"

//A class that extends the array class
//Keeps min and max values
template <class theType, class intType = int>
class CHArrayMinMax : public CHArray<theType, intType>
{
public:
	CHArrayMinMax(intType numPoints = 0) :
		CHArray<theType, intType>(numPoints){}

public:
	void AddPointMinMax(theType point)
	{
		if (IsEmpty()) min = max = point;
		else
		{
			if (point < min) min = point;
			if (point > max) max = point;
		}

		AddAndExtend(point);
	}

	theType CurMin() const { return min; }
	theType CurMax() const { return max; }

private:
	theType min;
	theType max;
};