
class CBufferStream:public IStream{
private:
	volatile LONG m_cRef;
	LARGE_INTEGER m_cur;
	LARGE_INTEGER m_size;
	void *		m_pbuff;
	BOOL m_bIsAttached;
public:
	CBufferStream():m_cRef(0),m_bIsAttached(FALSE), m_pbuff(0)
	{m_cur.QuadPart = 0LL;m_size.QuadPart = 0LL;}
	CBufferStream(void * pbuff, DWORD dwSizeOfByte):m_cRef(0),m_pbuff(pbuff),m_bIsAttached(TRUE)
	{m_size.QuadPart = dwSizeOfByte; m_cur.QuadPart = 0LL;}
	BOOL GetBuffer(void ** ppBuff, DWORD * pdwSize){
		if(!m_pbuff) return FALSE;
		if(ppBuff) *ppBuff = m_pbuff;
		if(pdwSize) *pdwSize = m_size.QuadPart;
		return TRUE;
	}
	~CBufferStream(){
		if(!m_bIsAttached && m_size.QuadPart){
			free(m_pbuff);
		}
	}
	ULONG _stdcall AddRef(){		
		return InterlockedIncrement(&m_cRef);
	}
	ULONG _stdcall Release(){		
		if(!InterlockedDecrement(&m_cRef)){
			delete this;
		}
		return m_cRef;
	}
	STDMETHOD(QueryInterface)(REFIID riid,void** pUnk){
		if(IsEqualIID(riid, IID_IUnknown)){
			*pUnk = (IUnknown*)reinterpret_cast<IUnknown*>(this);
			AddRef();			
			return S_OK;
		}
		else if(IsEqualIID(riid,IID_IStream)){
			*pUnk=(IStream*)reinterpret_cast<IStream*>(this);
			AddRef();
			return S_OK;
		}else
			return E_NOINTERFACE; 
	}
	///////////////////////////////////////////////////////////////////////////////////
	//函数名称：Seek
	//函数描述：定位内存操作
	//函数参数：...
	//返回  值：HRESULT
	/////////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
		if(dwOrigin == STREAM_SEEK_SET){
			//说明起始位置在开头dlibMove必须大于0
			if(dlibMove.QuadPart > m_size.QuadPart){
				return E_INVALIDARG;
			}
			m_cur.QuadPart = dlibMove.QuadPart;	
			if(plibNewPosition){
				(*plibNewPosition).QuadPart = m_cur.QuadPart;
			}
		}else if(dwOrigin == STREAM_SEEK_CUR){
			//说明起始位置在当前所指			
			if(dlibMove.QuadPart + m_cur.QuadPart > m_size.QuadPart){
				m_cur.QuadPart = 0;
				return E_INVALIDARG;
			}else{
				m_cur.QuadPart += dlibMove.QuadPart;
			}
			if(plibNewPosition){
				(*plibNewPosition).QuadPart = m_cur.QuadPart;
			}
		}else if(dwOrigin == STREAM_SEEK_END)
		{
			//说明起始位置在末尾，dlibMove必须小于0
			if(dlibMove.QuadPart + m_cur.QuadPart > m_size.QuadPart){
				m_cur.QuadPart = 0;
				return E_INVALIDARG;
			}else{
				m_cur.QuadPart += dlibMove.QuadPart;				
			}
			if(plibNewPosition){
				(*plibNewPosition).QuadPart = m_cur.QuadPart;
			}
		}
		else
			return E_INVALIDARG;

		return S_OK;
	}
	///////////////////////////////////////////////////////////////////////////////
	//函数名称：Write
	//函数描述：向缓冲区中写入
	//入口参数：无
	//返回  值：HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten){
		if(m_pbuff == NULL){
			m_pbuff = malloc(cb);
			ZeroMemory(m_pbuff, cb);
			m_size.QuadPart = cb;
		}else{
			if(m_cur.QuadPart + cb > m_size.QuadPart){
				m_size.QuadPart = m_cur.QuadPart + cb;
				m_pbuff = realloc(m_pbuff, m_size.QuadPart);
			}
		}
		
		memcpy((BYTE*)m_pbuff + m_cur.QuadPart, pv, cb);
		m_cur.QuadPart += cb;
		if(pcbWritten){
			*pcbWritten = cb;
		}
		return S_OK;
	}
	///////////////////////////////////////////////////////////////////////////////
	//函数名称：Read
	//函数描述：向缓冲区
	//入口参数：无
	//返回  值：HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Read)( void *pv, ULONG cb, ULONG *pcbRead)
	{
		if(!m_pbuff) return E_INVALIDARG;
		if(m_size.QuadPart < m_cur.QuadPart) return E_INVALIDARG;
		DWORD dwRead = (m_cur.QuadPart + cb > m_size.QuadPart)? m_size.QuadPart - m_cur.QuadPart:cb;
		memcpy(pv, (BYTE*)m_pbuff + m_cur.QuadPart, dwRead);
		if(pcbRead) *pcbRead = dwRead;
		m_cur.QuadPart += dwRead;
		return S_OK;
	}
	///////////////////////////////////////////////////////////////////////////////
	//函数名称：SetSize
	//函数描述：重新调整缓冲区大小
	//入口参数：无
	//返回  值：HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize){
		m_pbuff = realloc(m_pbuff, libNewSize.QuadPart);
		m_size.QuadPart = libNewSize.QuadPart;
		return S_OK;
	}
	///////////////////////////////////////////////////////////////////////////////
	//函数名称：CopyTo
	//函数描述：将缓冲区的内容copy到pstm
	//入口参数：无
	//返回  值：HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten){
		return pstm->Write(m_pbuff, cb.QuadPart, &pcbWritten->LowPart);
	}
	///////////////////////////////////////////////////////////////////////////////
	//函数名称：Clone
	//函数描述：将缓冲区的内容copy到pstm
	//入口参数：无
	//返回  值：HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Clone)( IStream **ppstm){return E_NOTIMPL;}
	STDMETHOD(Commit)(DWORD grfCommitFlags){return E_NOTIMPL;}
	STDMETHOD(Revert)( void){return E_NOTIMPL;}
	STDMETHOD(UnlockRegion)( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){return E_NOTIMPL;}
	STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag){return E_NOTIMPL;}
	STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){return E_NOTIMPL;}
};

	
	