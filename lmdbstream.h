
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
	//�������ƣ�Seek
	//������������λ�ڴ����
	//����������...
	//����  ֵ��HRESULT
	/////////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
		if(dwOrigin == STREAM_SEEK_SET){
			//˵����ʼλ���ڿ�ͷdlibMove�������0
			if(dlibMove.QuadPart > m_size.QuadPart){
				return E_INVALIDARG;
			}
			m_cur.QuadPart = dlibMove.QuadPart;	
			if(plibNewPosition){
				(*plibNewPosition).QuadPart = m_cur.QuadPart;
			}
		}else if(dwOrigin == STREAM_SEEK_CUR){
			//˵����ʼλ���ڵ�ǰ��ָ			
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
			//˵����ʼλ����ĩβ��dlibMove����С��0
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
	//�������ƣ�Write
	//�����������򻺳�����д��
	//��ڲ�������
	//����  ֵ��HRESULT
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
	//�������ƣ�Read
	//�����������򻺳���
	//��ڲ�������
	//����  ֵ��HRESULT
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
	//�������ƣ�SetSize
	//�������������µ�����������С
	//��ڲ�������
	//����  ֵ��HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize){
		m_pbuff = realloc(m_pbuff, libNewSize.QuadPart);
		m_size.QuadPart = libNewSize.QuadPart;
		return S_OK;
	}
	///////////////////////////////////////////////////////////////////////////////
	//�������ƣ�CopyTo
	//������������������������copy��pstm
	//��ڲ�������
	//����  ֵ��HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten){
		return pstm->Write(m_pbuff, cb.QuadPart, &pcbWritten->LowPart);
	}
	///////////////////////////////////////////////////////////////////////////////
	//�������ƣ�Clone
	//������������������������copy��pstm
	//��ڲ�������
	//����  ֵ��HRESULT
	///////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Clone)( IStream **ppstm){return E_NOTIMPL;}
	STDMETHOD(Commit)(DWORD grfCommitFlags){return E_NOTIMPL;}
	STDMETHOD(Revert)( void){return E_NOTIMPL;}
	STDMETHOD(UnlockRegion)( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){return E_NOTIMPL;}
	STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag){return E_NOTIMPL;}
	STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){return E_NOTIMPL;}
};

	
	