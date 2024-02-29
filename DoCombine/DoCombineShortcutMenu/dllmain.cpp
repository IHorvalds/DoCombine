// dllmain.cpp : Implementation of DllMain.

#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "DoCombineShortcutMenu_i.h"
#include "dllmain.h"

CDoCombineShortcutMenuModule _AtlModule;
HINSTANCE                    g_hInst_doCombineExt = 0;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst_doCombineExt = hInstance;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return _AtlModule.DllMain(dwReason, lpReserved);
}
