#pragma once
#include "../ByteBuffer.h"

class OdbcParameter
{
public:
	OdbcParameter(SQLSMALLINT parameterType, SQLSMALLINT dataType, SQLPOINTER parameterAddress, SQLLEN maxLength = 1);

	__forceinline SQLSMALLINT GetParameterType() { return m_parameterType; };
	__forceinline SQLSMALLINT GetDataType() { return m_dataType; };
	__forceinline SQLSMALLINT GetCDataType() { return m_cDataType; };
	__forceinline SQLPOINTER GetAddress() { return m_parameterAddress; };
	__forceinline SQLLEN GetDataTypeSize() { return m_dataTypeLength; };
	__forceinline SQLLEN *GetCBValue() { return &m_pCBValue; };

private:
	SQLSMALLINT m_parameterType, m_dataType, m_cDataType;
	SQLPOINTER m_parameterAddress;
	SQLLEN m_dataTypeLength, m_pCBValue;
};