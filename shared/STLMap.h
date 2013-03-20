#pragma once
#include <map>

#define foreach(itr, arr) for (auto itr = arr.begin(); itr != arr.end(); itr++)
#define foreach_stlmap(itr, arr) for (auto itr = arr.m_UserTypeMap.begin(); itr != arr.m_UserTypeMap.end(); itr++)
#define foreach_array(itr, arr) foreach_array_n(itr, arr, sizeof(arr) / sizeof(arr[0]))
#define foreach_array_n(itr, arr, len) for (auto itr = 0; itr < len; itr++)
#define foreach_region(x, z) for (int x = -1; x <= 1; x++) \
	for (int z = -1; z <= 1; z++)

template <class T> 
class CSTLMap  
{
public:
	typedef typename std::map<uint32, T*>::iterator Iterator;
	std::map<uint32, T*> m_UserTypeMap;
	FastMutex m_lock;

	int GetSize()
	{
		FastGuard lock(m_lock);
		return m_UserTypeMap.size(); 
	}

	bool IsExist(uint32 key)
	{
		FastGuard lock(m_lock);
		return (m_UserTypeMap.find(key) != m_UserTypeMap.end()); 
	}

	bool IsEmpty() 
	{ 
		FastGuard lock(m_lock);
		return m_UserTypeMap.empty(); 
	}

	bool PutData(uint32 key_value, T* pData) 
	{
		FastGuard lock(m_lock);
		return m_UserTypeMap.insert(std::make_pair(key_value, pData)).second; 
	}

	T* GetData(uint32 key_value)
	{
		FastGuard lock(m_lock);
		T *result = NULL;
		auto itr = m_UserTypeMap.find(key_value);
		if (itr != m_UserTypeMap.end())
			result = itr->second;
		return result;
	}

	void DeleteData(long key_value)
	{
		FastGuard lock(m_lock);
		auto itr = m_UserTypeMap.find(key_value);
		if (itr!= m_UserTypeMap.end())
		{
			delete itr->second;
			m_UserTypeMap.erase(itr);
		}
	}

	void DeleteAllData()
	{
		FastGuard lock(m_lock);
		if (m_UserTypeMap.empty())
			return;

		foreach (itr, m_UserTypeMap)
			delete itr->second;

		m_UserTypeMap.clear();
	}

	~CSTLMap() { DeleteAllData(); };

};