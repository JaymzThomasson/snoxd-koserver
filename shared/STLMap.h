#pragma once
#include <map>

#define foreach(itr, arr) for (auto itr = arr.begin(); itr != arr.end(); itr++)
#define foreach_stlmap(itr, arr) for (auto itr = arr.m_UserTypeMap.begin(); itr != arr.m_UserTypeMap.end(); itr++)
#define foreach_array(itr, arr) foreach_array_n(itr, arr, sizeof(arr) / sizeof(arr[0]))
#define foreach_array_n(itr, arr, len) for (auto itr = 0; itr < len; itr++)
#define foreach_region(x, z) for (int x = -1; x <= 1; x++) \
	for (int z = -1; z <= 1; z++)

template <class T> class CSTLMap  
{
public:
	typedef typename std::map<long, T*>::iterator Iterator;
	std::map<long, T*> m_UserTypeMap;
	
	int GetSize() { return m_UserTypeMap.size(); };
	bool IsExist(int key_value)  { return (m_UserTypeMap.find(key_value) != m_UserTypeMap.end()); };

	bool IsEmpty() { return m_UserTypeMap.empty(); };
	bool PutData(long key_value, T* pData) { return m_UserTypeMap.insert(std::make_pair(key_value, pData)).second; };

	T* GetData(long key_value)
	{
		T *result = NULL;
		auto itr = m_UserTypeMap.find(key_value);
		if (itr != m_UserTypeMap.end())
			result = itr->second;
		return result;
	};

	void DeleteData(long key_value)
	{
		auto itr = m_UserTypeMap.find(key_value);
		if (itr!= m_UserTypeMap.end())
		{
			delete itr->second;
			m_UserTypeMap.erase(itr);
		}
	};

	void DeleteAllData()
	{
		if (IsEmpty())
			return;

		foreach (itr, m_UserTypeMap)
			delete itr->second;
		m_UserTypeMap.clear();
	};

	~CSTLMap() { DeleteAllData(); };

};