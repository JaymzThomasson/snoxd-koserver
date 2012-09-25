#pragma once

// Our template version of MFC's IMPLEMENT_DYNAMIC
#define IMPLEMENT_TEMPLATE_DYNAMIC(class_name, base_class_name) \
	template <class T> AFX_COMDAT const CRuntimeClass class_name<T>::class##class_name = { \
		#class_name, sizeof(class class_name), 0xFFFF, NULL, \
			RUNTIME_CLASS(base_class_name), NULL, NULL }; \
	template <class T> CRuntimeClass* class_name<T>::GetRuntimeClass() const \
		{ return RUNTIME_CLASS(class_name); }

// Somewhat hacky, but MFC works awkwardly enough that this is required.
#define COPY_ROW() new T(); memcpy(data, &m_data, sizeof(T));

// Cheap right trim, used for strings. We should do this better.
#define TRIM_RIGHT(v) { CString str = v; str.TrimRight(); strcpy(v, str); }

class DummyStorage
{
};

template <class T>
class CMyRecordSet : public CRecordset
{
public:
	CMyRecordSet(CDatabase* pDatabase = NULL) : CRecordset(pDatabase)
	{
		m_nDefaultType = snapshot;
	}

	DECLARE_DYNAMIC(CMyRecordSet)

	virtual CString GetDefaultConnect() { return _T("ODBC;DSN=KN_Online;UID=knight;PWD=knight"); };
	virtual CString GetDefaultSQL() { return _T(""); };
	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		// placeholder
	};

private:
	bool AttemptOpen(bool & isEmpty, bool bAllowEmptyTable = false)
	{
		if (!Open())
		{
			AfxMessageBox(_T(GetDefaultSQL() + " could not be opened!"));
			return false;
		}
		else if (IsBOF() || IsEOF())
		{
			isEmpty = true;
			if (bAllowEmptyTable)
				return false;

			AfxMessageBox(_T(GetDefaultSQL() + " is empty!"));
			return false;
		}

		MoveFirst();
		return true;
	};

public:
	bool Read(bool bAllowEmptyTable = false)
	{
		bool isEmpty = false;
		if (!AttemptOpen(isEmpty, bAllowEmptyTable))
			return isEmpty && bAllowEmptyTable;

		while (!IsEOF())
		{
			HandleRead();
			MoveNext();
		}
		return true;
	};

	virtual void HandleRead()
	{
		// placeholder
	};

protected:
	T m_data;
};

IMPLEMENT_TEMPLATE_DYNAMIC(CMyRecordSet, CRecordset)