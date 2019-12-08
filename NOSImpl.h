#pragma once
#include "..\..\include\nos.h"

class CNOSImpl:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNOSImpl>,
	public INoSql
{
public:
	CNOSImpl(void);
	virtual ~CNOSImpl(void);

	DECLARE_NO_REGISTRY()
	BEGIN_COM_MAP(CNOSImpl)
		COM_INTERFACE_ENTRY(INoSql)
	END_COM_MAP()

	STDMETHOD(Init)(LPCWSTR lpszNamespace, LPCWSTR lpszWorkDir, int nDbSizeInMB);;
	STDMETHOD(Subscript)(LPCWSTR lpszKey);
	STDMETHOD(SetDispatcher)(IUnknown * pUnk);
	STDMETHOD(SetValue)(LPCWSTR lpszKey, VARIANT * pVar);
	STDMETHOD(GetValue)(LPCWSTR lpszKey, VARIANT * pVar);
private:
	struct tagNOSImplData * _p;
	struct tagNOSImplData & data;
};

OBJECT_ENTRY_AUTO(__uuidof(ClassNoSql), CNOSImpl)