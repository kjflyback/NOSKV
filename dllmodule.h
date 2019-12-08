

#ifndef dllmodule_h__
#define dllmodule_h__

class CNOSModule : public CAtlDllModuleT< CNOSModule >
{
public :
	HINSTANCE _hInst;
	CNOSModule():_hInst(NULL){}
	~CNOSModule(){}
};

#endif // dllmodule_h__

