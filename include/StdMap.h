#pragma once

#include <map>
#include "Array.h"
#include "Savable.h"

template<class keyType, class valType = int>
class StdMap : public std::map<keyType,valType>, public Savable
{
public:
	void Serialize(BArchive& ar)
	{
		std::map<keyType, valType>* pMap = (std::map<keyType, valType>*) this;

		CHArray<keyType> keys(Count());
		CHArray<valType> vals(Count());

		if (ar.IsStoring())
		{
			for (auto& curPair : *pMap)
			{
				keys.AddPoint(curPair.first);
				vals.AddPoint(curPair.second);
			}

			ar & keys & vals;
		}
		else
		{
			ar & keys & vals;
			Clear();
			for (int i = 0; i < keys.Count(); i++) Insert(keys[i], vals[i]);
		}
			
	}

	void Insert(const keyType& key)		//Insert just a key - the value is assumed to not matter and set to 1
	{
		(*this)[key]=(valType)1;
	}

	void Insert(const keyType& key, const valType& val)		//Insert a value and a key
	{
		(*this)[key]=val;
	}

	//Insert only keys from CHArray
	template<class argIntType>
	void InsertFromArray(const CHArray<keyType,argIntType>& keyArray)
	{
		for(argIntType i=0; i < keyArray.Count(); i++)	Insert(keyArray.arr[i]);
	}

	//Insert keys and vals from CHArrays
	template<class argIntType>
	void InsertFromArrays(const CHArray<keyType, argIntType>& keyArray,
							const CHArray<valType, argIntType>& valArray)	
	{
		for (argIntType i = 0; i < keyArray.Count(); i++)	Insert(keyArray.arr[i], valArray.arr[i]);
	}

	void Remove(const keyType& key)		{erase(key);}

	void Clear()	{clear();}

	bool IsEmpty() const	{return empty();}

	bool IsPresent(const keyType& key) const
	{
		return (count(key) > 0);
	};

	int Count() const
	{
		return size();
	}

	//Is one of array elements present
	template<class argIntType>
	bool IsPresentOneOf(const CHArray<keyType,argIntType>& theArray) const
	{
		for(argIntType i=0; i < theArray.Count(); i++)
		{
			if(IsPresent(theArray.arr[i])) return true;
		}

		return false;
	}
};