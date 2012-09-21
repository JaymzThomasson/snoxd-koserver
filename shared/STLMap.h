// STLMap.h: interface for the CSTLMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STLMap_H__9153F571_6888_4BD4_ABC9_E97416B54096__INCLUDED_)
#define AFX_STLMap_H__9153F571_6888_4BD4_ABC9_E97416B54096__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable : 4786)

#include <map>
using namespace std;

template <class T> class CSTLMap  
{
public:
	typedef typename map < int, T* >::iterator		Iterator;
	typedef typename map < int, T* >::value_type		ValueType;

	map < int, T* >		m_UserTypeMap;
	

	int GetSize() { return m_UserTypeMap.size(); };
	
	bool PutData( int key_value, T* pData)
	{
		pair<Iterator, bool> temp_pair = m_UserTypeMap.insert( ValueType( key_value, pData ) );
		return temp_pair.second;
	};

	T* GetData( int key_value )
	{
		T* pData = NULL;
		Iterator iter = m_UserTypeMap.find( key_value );
		if( iter == m_UserTypeMap.end() )
			return NULL;
		else
			pData = (*iter).second;
		
		return pData;
	};

	Iterator DeleteData( int key_value )
	{
		if( m_UserTypeMap.empty() )
			return m_UserTypeMap.end();
		
		Iterator iter = m_UserTypeMap.find( key_value );
		if( iter == m_UserTypeMap.end() )
			return m_UserTypeMap.end();
		else {
			T* pData = (*iter).second;

			iter = m_UserTypeMap.erase(iter);

			delete pData;
			pData = NULL;

			return iter;
		}
	};

	void DeleteAllData()
	{
		Iterator iter1 = m_UserTypeMap.begin(), iter2 = m_UserTypeMap.end();
		for(; iter1 != iter2; iter1++ )
			delete (*iter1).second;

		Clear();
	};

	void Clear()
	{
		m_UserTypeMap.clear();
	};

	bool IsExist( int key_value )
	{
		return (m_UserTypeMap.find(key_value) != m_UserTypeMap.end())
	};

	bool IsEmpty() { return m_UserTypeMap.empty(); };

	CSTLMap() {};
	virtual ~CSTLMap() {
		DeleteAllData();
	};
};

#define foreach(itr, arr) for (auto itr = arr.begin(); itr != arr.end(); itr++)
#define foreach_stlmap(itr, arr) for (auto itr = arr.m_UserTypeMap.begin(); itr != arr.m_UserTypeMap.end(); itr++)

#endif // !defined(AFX_STLMap_H__9153F571_6888_4BD4_ABC9_E97416B54096__INCLUDED_)
