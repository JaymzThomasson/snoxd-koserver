#pragma once
#include <map>

#define foreach(itr, arr) for (auto itr = arr.begin(); itr != arr.end(); itr++)
#define foreach_stlmap(itr, arr) for (auto itr = arr.m_UserTypeMap.begin(); itr != arr.m_UserTypeMap.end(); itr++)
#define foreach_array(itr, arr) foreach_array_n(itr, arr, sizeof(arr) / sizeof(arr[0]))
#define foreach_array_n(itr, arr, len) auto itr ## Value = arr[0]; for (auto itr = 0; itr < len; itr++, itr ## Value = arr[itr])

template <class T> class CSTLMap  
{
public:
	typedef typename std::map<long, T*>::iterator Iterator;
	std::map<long, T*> m_UserTypeMap;
	
	int GetSize() { return m_UserTypeMap.size(); };
	bool IsExist(long key_value) { return (m_UserTypeMap.find(key_value) != m_UserTypeMap.end()) };
	bool IsEmpty() { return m_UserTypeMap.empty(); };
	bool PutData(long key_value, T* pData) { return m_UserTypeMap.insert(std::make_pair(key_value, pData)).second; };

	T* GetData(long key_value)
	{
		Iterator iter = m_UserTypeMap.find(key_value);
		if (iter == m_UserTypeMap.end())
			return NULL;

		return (*iter).second;
	};

	Iterator DeleteData(long key_value)
	{
		Iterator iter = m_UserTypeMap.find(key_value);
		if (iter == m_UserTypeMap.end())
			return m_UserTypeMap.end();

		delete (*iter).second;
		return m_UserTypeMap.erase(iter);
	};

	void DeleteAllData()
	{
		foreach (itr, m_UserTypeMap)
			delete itr->second;

		Clear();
	};

	void Clear() { m_UserTypeMap.clear(); };
	~CSTLMap() { DeleteAllData(); };
};