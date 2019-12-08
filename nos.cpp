// nos.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"



// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	// HRESULT hr = _AtlModule.DllRegisterServer();
	// return hr;
	return E_FAIL;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	// HRESULT hr = _AtlModule.DllUnregisterServer();
	// return hr;
	return E_FAIL;
}