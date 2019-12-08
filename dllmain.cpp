// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "dllmodule.h"

CNOSModule _AtlModule;

extern "C" BOOL WINAPI DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	_AtlModule._hInst = hModule;
	return _AtlModule.DllMain(ul_reason_for_call, lpReserved);
}

