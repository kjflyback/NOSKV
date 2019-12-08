#include "StdAfx.h"
#include "NOSImpl.h"
#include "..\CPPLMDb.h"
#include "lmdbstream.h"
#include <string>
#include <vector>
#include <map>

typedef std::map<ULONGLONG, std::wstring> ULonglongStringArray;
static ULONGLONG
NotifyKVhash(const void *val, size_t len)
{
	const unsigned char *s = (const unsigned char *) val, *end = s + len;
	ULONGLONG hval = 0xcbf29ce484222325ULL;
	/*
	 * FNV-1a hash each octet of the buffer
	 */
	while (s < end) {
		hval = (hval ^ *s++) * 0x100000001b3ULL;
	}
	/* return our new hash value */
	return hval;
}

#define NOTIFYEVENT L"Global\\{74A8E908-470E-429a-9F66-F64103E5E475}.notify"
#define GLOBALMMP L"Global\\{74A8E908-470E-429a-9F66-F64103E5E475}.mmp"
#define GLOBALMUTEXDBINIT L"Global\\{74A8E908-470E-429a-9F66-F64103E5E475}.dbinit"
#define GLOBALNAMESPACE L"Global\\{74A8E908-470E-429a-9F66-F64103E5E475}"

#pragma pack(push, 8)
typedef struct tagSubscriptNode{
	ULONGLONG clientid;
	ULONGLONG key;
	struct{
		char use;
		char valid;
	}keyuse;
}SUBSCRIPTNODE;
#pragma pack(pop)

class CNotifyKV{
public:
	class Notify{
	public:
		virtual void OnValueChanged(BOOL TimeOutOrEvent, LPCWSTR lpszKey) = 0;
		~Notify(){}
	};
	CNotifyKV(LPCWSTR lpszNameSpace, LPCWSTR lpszDir, DWORD dwSizeOfSubscriptNode/* = 1024 * 1024*/): // 1024 * 1024 / 24 = 43690 keys
	m_pNotify(NULL), m_pEntry(NULL)
	{
		if(!dwSizeOfSubscriptNode)
			dwSizeOfSubscriptNode = 1024 * 1024 * 10;

		CoCreateGuid(&m_clientid);
		m_clienthash = NotifyKVhash((void*)&m_clientid, sizeof(GUID));

		SECURITY_ATTRIBUTES sa = {0};
		SECURITY_DESCRIPTOR sd = {0};

		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, NULL, TRUE);
		sa.lpSecurityDescriptor = &sd;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;

		wcscpy(m_szNamespace, lpszNameSpace);

		wchar_t szDir[MAX_PATH] = {0};
		wchar_t szFileName[MAX_PATH] = {0};
		wchar_t fileName[MAX_PATH] = {0};
		
		wsprintf(fileName, NOTIFYEVENT L"-%s", lpszNameSpace);
		m_event = CreateSemaphore(&sa, 0, 10000, fileName);

		wsprintf(fileName, GLOBALMUTEXDBINIT L"-%s", lpszNameSpace);
		BOOL bIsFirst = FALSE;
		m_firstStartup = CreateMutex(&sa, TRUE, fileName);
		if(ERROR_ALREADY_EXISTS != GetLastError()){
			bIsFirst = TRUE;
			if(lpszDir){
				wcscpy(szDir, lpszDir);
			}else{

				GetModuleFileName(NULL, szFileName, MAX_PATH);
				PathRemoveFileSpec(szFileName);
				wcscpy(szDir, szFileName);
			}

			wsprintf(fileName, L"%s\\%s.nos",szDir, lpszNameSpace);
			DeleteFile(fileName);
			wsprintf(fileName, L"%s\\%s.nos-lock",szDir, lpszNameSpace);
			DeleteFile(fileName);

			wsprintf(fileName, GLOBALMMP L"-%s", lpszNameSpace);

			m_mmap = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, dwSizeOfSubscriptNode, fileName);

		}else{
			wsprintf(fileName, GLOBALMMP L"-%s", lpszNameSpace);
			m_mmap = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, fileName);
			// m_mmap = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, dwSizeOfSubscriptNode, fileName);
		}
		
		if(GetLastError() == ERROR_ACCESS_DENIED){
			return;
		}
		BYTE * pData = (BYTE*)MapViewOfFile(m_mmap, FILE_MAP_ALL_ACCESS, 0, 0, dwSizeOfSubscriptNode);
		m_pEntry = pData;

		wsprintf(szFileName, L"%s\\%s.nos",szDir, lpszNameSpace);

		if(bIsFirst){
			ZeroMemory(pData, dwSizeOfSubscriptNode);
			memcpy(pData, szFileName, wcslen(szFileName) * sizeof(wchar_t));
		}else{
			wcscpy(szFileName, (wchar_t*)pData);
		}
		wcscpy(fileName, szFileName);
		// wsprintf(fileName, L"%s.db", lpszNameSpace);
		HRESULT hr = m_db.init(fileName, 1024, 0x94000); // 1G size
		ATLASSERT(SUCCEEDED(hr));
		m_db.OpenDB();

		m_psnode = (SUBSCRIPTNODE*)((BYTE*)pData + 0xff * sizeof(wchar_t));

		struct foo{
			static void WINAPI Run(LPVOID lpvoid, BOOLEAN bTimerOrEvent){
				CNotifyKV * pThis = (CNotifyKV*)lpvoid;
				pThis->Check(bTimerOrEvent);

			}
		};

		m_wait = RegisterWaitForSingleObjectEx(m_event, &foo::Run, this, 1000, WT_EXECUTEDEFAULT); // 1s check once
		wsprintf(fileName, GLOBALNAMESPACE L"-SubscriptKey-%s", lpszNameSpace);
		m_subscriptEvent = CreateEvent(&sa, FALSE, TRUE, fileName);
	}
	~CNotifyKV(){
		UnregisterWait(m_wait);
		if(m_pEntry)
			UnmapViewOfFile(m_pEntry);
		CloseHandle(m_event);
	}
	void SetObserver(Notify * pNotify){
		m_pNotify = pNotify;
	}
	void Check(BOOL bTimeOutOrEvent){
		if(!m_pNotify) return;
		SUBSCRIPTNODE * pPos = m_psnode;
		while(pPos->keyuse.valid){
			if(pPos->keyuse.use && pPos->clientid == m_clienthash){
				pPos->keyuse.use = 0;
				ULonglongStringArray::iterator it = m_llsa.find(pPos->key);
				if(it == m_llsa.end()) continue;

				m_pNotify->OnValueChanged(bTimeOutOrEvent, (*it).second.c_str());

			}
			pPos++;
		}
	}

	struct _inbuff{
		VARTYPE vt;
		char * buff[1];
	};
	void Set(LPCWSTR lpszKey, VARIANT * pvar){
		CppLMDb::transblock tb(m_db);
		CComVariant var;
		VARTYPE vt = pvar->vt;
		var.Attach(pvar);
		var.ChangeType(VT_BSTR);
		DWORD dwSize = sizeof(VARTYPE) + sizeof(wchar_t) * (1 + wcslen(var.bstrVal));
		DWORD dwTimes = dwSize / 4;
		dwSize = (dwTimes + 1) * 4;
		BYTE * pTmp = new BYTE[dwSize];
		ZeroMemory(pTmp, dwSize);
		struct _inbuff *pbuff = (struct _inbuff*)pTmp;

		// memcpy((void*)pbuff->buff, (void*)var.bstrVal, dwSize);
		pbuff->vt = vt;
		wcscpy((wchar_t*)pbuff->buff, var.bstrVal);
		m_db.Set(&tb, (void*)lpszKey, sizeof(wchar_t) * wcslen(lpszKey),
			pbuff, dwSize);
		delete [] pTmp;
		NotifyKey(lpszKey);
	}
	BOOL Get(LPCWSTR lpszKey, VARIANT * pvar){
		CppLMDb::transblock tb(m_db);
		struct _inbuff * pbuff = NULL;
		DWORD dwLen = 0;
		HRESULT hr = m_db.Get(&tb, (void*)lpszKey, sizeof(wchar_t) * wcslen(lpszKey),
			(void**)&pbuff, &dwLen);
		if(!dwLen) return FALSE;
		CComVariant var = (wchar_t*)pbuff->buff;
		var.ChangeType(pbuff->vt);
		VariantCopy(pvar, &var);
		return TRUE;
	}
	void SubscriptKey(LPCWSTR lpszKey){
		WaitForSingleObject(m_subscriptEvent, 2000); // wait for 2s
		SUBSCRIPTNODE sn = {0};
		sn.clientid = m_clienthash;
		sn.key = NotifyKVhash((void*)lpszKey, sizeof(wchar_t) * wcslen(lpszKey));
		sn.keyuse.use = 0;
		sn.keyuse.valid = 1;
		BOOL bIsExists = FALSE;
		SUBSCRIPTNODE * pPos = m_psnode;
		while(pPos->keyuse.valid){
			if(sn.clientid == pPos->clientid && sn.key == pPos->key) // already exists key
			{
				bIsExists = TRUE;
				pPos->keyuse.valid = 1;
				break;
			}
			pPos++;
		}
		if(!bIsExists)
			*pPos = sn;
		SetEvent(m_subscriptEvent);
		m_llsa[sn.key] = lpszKey;
	}
	void NotifyKey(LPCWSTR lpszKey){

		ULONGLONG key = NotifyKVhash((void*)lpszKey, sizeof(wchar_t) * wcslen(lpszKey));
		SUBSCRIPTNODE * pPos = m_psnode;
		// WaitForSingleObject(m_subscriptEvent, 1000); // wait for 2s
		typedef std::map<ULONGLONG, BOOL> ULONGLONGArray;
		ULONGLONGArray lla;

		int nCountClient = 0;
		while(pPos->keyuse.valid){
			if(pPos->clientid != m_clienthash){
				lla[pPos->clientid] = TRUE;
				// all node with key 
				if(key == pPos->key && !pPos->keyuse.use){ // not for self
					pPos->keyuse.use = 1;
				}
			}
			pPos++;
		}
		// SetEvent(m_subscriptEvent);
		ReleaseSemaphore(m_event, lla.size(), NULL);
		Sleep(0);
	}
private:
	BYTE * m_pEntry;
	HANDLE m_firstStartup;
	ULonglongStringArray m_llsa;
	Notify * m_pNotify;
	HANDLE m_event;
	HANDLE m_wait;
	HANDLE m_mmap;
	HANDLE m_subscriptEvent;
	wchar_t m_szNamespace[MAX_PATH];
	SUBSCRIPTNODE * m_psnode;
	GUID   m_clientid;
	ULONGLONG m_clienthash;
	CppLMDb m_db;
};

class Dispatcher:public CNotifyKV::Notify{
public:
	CComQIPtr<INoSqlDispatcher> spDispatcher;
	CComQIPtr<INoSql>			spMaster;
	Dispatcher(){}
	~Dispatcher(){}
	void SetDelegate(IUnknown * p, IUnknown * pMaster){
		spDispatcher = p;
		spMaster = pMaster;
	}
	void OnValueChanged(BOOL b, LPCWSTR lpszKey){
		if(!spDispatcher) return;
		spDispatcher->OnChanged(lpszKey, spMaster, b);
	}
};

typedef struct tagNOSImplData{
	CNotifyKV * _kv;
	Dispatcher dispatcher;
}NOSIMPLDATA;
CNOSImpl::CNOSImpl( void ):
_p(new NOSIMPLDATA),
data(*_p)
{
	data._kv = NULL;
}
CNOSImpl::~CNOSImpl( void )
{
	if(data._kv){
		delete data._kv;
		data._kv = NULL;
	}
	delete _p;
}
STDMETHODIMP CNOSImpl::Init( LPCWSTR lpszNamespace, LPCWSTR lpszWorkDir, int nDbSizeInMB )
{
	CNotifyKV * pKv = new CNotifyKV(lpszNamespace, lpszWorkDir, nDbSizeInMB);
	pKv->SetObserver(&data.dispatcher);
	data._kv = pKv;
	return S_OK;
}

STDMETHODIMP CNOSImpl::Subscript( LPCWSTR lpszKey )
{
	if(!data._kv) return E_NOTIMPL;

	data._kv->SubscriptKey(lpszKey);
	return S_OK;
}

STDMETHODIMP CNOSImpl::SetDispatcher( IUnknown * pUnk )
{
	data.dispatcher.SetDelegate(pUnk, GetUnknown());
	return S_OK;
}

STDMETHODIMP CNOSImpl::SetValue( LPCWSTR lpszKey, VARIANT * pVar )
{
	if(!data._kv) return E_NOTIMPL;
	data._kv->Set(lpszKey, pVar);
	return S_OK;
}

STDMETHODIMP CNOSImpl::GetValue( LPCWSTR lpszKey, VARIANT * pVar )
{
	if(!data._kv) return E_NOTIMPL;
	return data._kv->Get(lpszKey, pVar)?S_OK:E_INVALIDARG;
}




