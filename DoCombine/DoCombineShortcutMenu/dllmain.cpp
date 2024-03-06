// dllmain.cpp : Implementation of DllMain.

#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "DoCombineShortcutMenu_i.h"
#include "dllmain.h"
#include "logging.h"

CDoCombineShortcutMenuModule _AtlModule;
HINSTANCE                    g_hInst_doCombineExt = 0;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst_doCombineExt = hInstance;
        LOGGER.info("DoCombineShortcutMenu.dll loaded");
        break;
    case DLL_PROCESS_DETACH:
        LOGGER.info("DoCombineShortcutMenu.dll unloaded");
        break;
    }
    return _AtlModule.DllMain(dwReason, lpReserved);
}
