#ifndef nos_h__
#define nos_h__


MIDL_INTERFACE("{471E0AE7-26DE-4410-AC11-89DCE118F771}")
INoSqlDispatcher:public IUnknown{
	STDMETHOD(OnChanged)(LPCWSTR lpszKey, IUnknown * pMaster, BOOL bTEvent);
};

MIDL_INTERFACE("{C01256B3-646E-4f85-A574-5944B23151AF}")
INoSql: public IUnknown{
	STDMETHOD(Init)(LPCWSTR lpszNamespace, LPCWSTR lpszWorkDir, int nDbSizeInMB);
	STDMETHOD(Subscript)(LPCWSTR lpszKey);
	STDMETHOD(SetDispatcher)(IUnknown * pUnk);
	STDMETHOD(SetValue)(LPCWSTR lpszKey, VARIANT * pVar);
	STDMETHOD(GetValue)(LPCWSTR lpszKey, VARIANT * pVar);
};

MIDL_INTERFACE("{9F6FDBF7-ADE3-44ee-838E-D9A292BF7EBA}") ClassNoSql;
#endif // nos_h__