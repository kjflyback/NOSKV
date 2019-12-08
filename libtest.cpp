// libtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <atlbase.h>
#include <iostream>
#include <map>
#include <string>
#include <atlbase.h>
#include <atlwin.h>
#include <atlapp.h>
#include "aclapi.h"
#include <sddl.h>
#include <iostream>
#include "../../include/nos.h"
class CDataDispatcher:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDataDispatcher>,
	public INoSqlDispatcher
{
public:
	CDataDispatcher(){}
	~CDataDispatcher(){}

	DECLARE_NO_REGISTRY()
	BEGIN_COM_MAP(CDataDispatcher)
		COM_INTERFACE_ENTRY(INoSqlDispatcher)
	END_COM_MAP()

	STDMETHOD(OnChanged)(LPCWSTR lpszKey, IUnknown * pMaster, BOOL b){
		CComQIPtr<INoSql> spMaster = pMaster;
		CComVariant var;
		spMaster->GetValue(lpszKey, &var);
		var.ChangeType(VT_BSTR);
		printf("%s key:%ws changed(%ws)\n",b?"timeout":"event", lpszKey, var.bstrVal);
		return S_OK;

	}

};

CComModule _Module;
int _tmain(int argc, _TCHAR* argv[])
{
	CoInitialize(NULL);
// 	class Notifyer:public CNotifyKV::Notify{
// 	public:
// 		void OnValueChanged(LPCWSTR lpszKey){
// 			// printf("key:%ws changed\n", lpszKey);
// 			// Sleep(0);
// 		}
// 	}notifyer;
	
// 	CNotifyKV kv2(L"kv");
// 	kv2.SetObserver(&notifyer);

	HMODULE h = LoadLibrary(L"nos.dll");
	// createinstall
	LPFNGETCLASSOBJECT pGet = (LPFNGETCLASSOBJECT)GetProcAddress(h, "DllGetClassObject");
	ATLASSERT(pGet);
	if(!pGet){
		return GetLastError();
	}
	CComQIPtr<IClassFactory> spFactory;
	HRESULT hr = pGet(__uuidof(ClassNoSql), IID_IClassFactory, (void**)&spFactory);
	if(FAILED(hr)){
		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}
	ATLASSERT(spFactory);
	if(!spFactory) return E_UNEXPECTED;

	CComQIPtr<INoSql> spNoSql;
	if(FAILED(hr = spFactory->CreateInstance(NULL, __uuidof(INoSql), (void**)&spNoSql))){
		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	if(!spNoSql){
		return -1;
	}

	spNoSql->Init(L"kv", L"D:\\", 0);
	
	CComObject<CDataDispatcher> * pDispatcher;
	CComObject<CDataDispatcher>::CreateInstance(&pDispatcher);
	CComQIPtr<INoSqlDispatcher> spDispatcher = pDispatcher;
	spNoSql->SetDispatcher(spDispatcher);


	BOOL bFirstCmd = TRUE;
	
	while(1){
		wchar_t szLine[MAX_PATH] = {0};
		if(bFirstCmd){
			bFirstCmd = FALSE;
			if(argc > 1){
				wchar_t * pFind = GetCommandLine();
				pFind += wcslen(argv[0]) + 2;
				wcscpy(szLine, pFind);
			}else
				continue;
		}else
			std::wcin.getline(szLine, MAX_PATH);
		wchar_t szCmd[MAX_PATH] = {0}, szKey[MAX_PATH] = {0}, szValue[MAX_PATH] = {0}, szType[MAX_PATH] = {0};
		int nAnalyze = swscanf(szLine, L"%s %[^=]=%[^:]:%[^\n]", szCmd, szKey, szValue, szType);
		
		if(!wcsicmp(szCmd, L"set")){
			CComVariant varVal = szValue;
			if(nAnalyze == 4 && !wcsicmp(szType, L"int")) varVal.ChangeType(VT_INT);
			spNoSql->SetValue(szKey, &varVal);
		}else if(!wcsicmp(szCmd, L"get")){
			CComVariant varVal;
			spNoSql->GetValue(szKey, &varVal);
			std::wcout<<L"get "<<szKey<<L" --- ";
			switch(varVal.vt){
				case VT_I1:case VT_UI1: std::wcout<<L"char:"<<varVal.bVal<<std::endl;break;
				case VT_I2:case VT_UI2: std::wcout<<L"short:"<<varVal.uiVal<<std::endl;break;
				case VT_I4:case VT_UI4:case VT_UINT:case VT_INT: std::wcout<<L"int:"<<varVal.intVal<<std::endl;break;
				case VT_BSTR: std::wcout<<L"string:"<<varVal.bstrVal<<std::endl; break;
				default:
					std::wcout<<L"varianttype:"<<varVal.vt;
					varVal.ChangeType(VT_BSTR);
					std::wcout<<varVal.bstrVal<<std::endl;
					break;
			}
		}else if(!wcsicmp(szCmd, L"sub")){
			spNoSql->Subscript(szKey);
		}else if(!wcsicmp(szCmd, L"test")){
			for(int i = 0;i<1000;i++){
				spNoSql->SetValue(L"test", &CComVariant(i));
				Sleep(0);
			}
		}else if(!wcsicmp(szCmd, L"quit")){
			break;
		}else{
			std::wcout<<L"command should be 'set a=b:int|string' or 'sub key'\n";
			continue;
		}
	}

	return 0;
}

