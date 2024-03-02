// DCContextMenuHandler.h : Declaration of the CDCContextMenuHandler

#pragma once
#define ID_MODIFY_DOCS 0
#define MODIFY_DOCS_VERBW L"modify"

#include "pch.h"
#include "resource.h" // main symbols

#include "DoCombineShortcutMenu_i.h"
#include <string>

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error                                                                                                                 \
    "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CDCContextMenuHandler

class ATL_NO_VTABLE DECLSPEC_UUID("73b668a5-0434-4983-bb8a-8fab7c728e64") CDCContextMenuHandler
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CDCContextMenuHandler, &CLSID_DCContextMenuHandler>,
      public IShellExtInit,
      public IContextMenu,
      public IExplorerCommand
{
public:
    DECLARE_REGISTRY_RESOURCEID(106)
    DECLARE_NOT_AGGREGATABLE(CDCContextMenuHandler)

    BEGIN_COM_MAP(CDCContextMenuHandler)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IContextMenu)
    COM_INTERFACE_ENTRY(IExplorerCommand)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct() { return S_OK; }

    void FinalRelease() {}

    CDCContextMenuHandler();
    ~CDCContextMenuHandler();

    // IShellExtInit
    HRESULT STDMETHODCALLTYPE Initialize(_In_opt_ PCIDLIST_ABSOLUTE pidlFolder, _In_opt_ IDataObject* pdtobj,
                                         _In_opt_ HKEY hkeyProgID);

    // IContextMenu
    HRESULT STDMETHODCALLTYPE QueryContextMenu(_In_ HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast,
                                               UINT uFlags);
    HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR idCmd, UINT uType, _In_ UINT* pReserved, LPSTR pszName,
                                               UINT cchMax);
    HRESULT STDMETHODCALLTYPE InvokeCommand(_In_ CMINVOKECOMMANDINFO* pici);

    // IExplorerCommand
    virtual STDMETHODIMP GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName) override;
    virtual STDMETHODIMP GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon) override;
    virtual STDMETHODIMP GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip) override;
    virtual STDMETHODIMP GetCanonicalName(GUID* pguidCommandName) override;
    virtual STDMETHODIMP GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState) override;
    virtual STDMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) override;
    virtual STDMETHODIMP GetFlags(EXPCMDFLAGS* pFlags) override;
    virtual STDMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum) override;

private:
    void    Uninitialize();
    HRESULT LaunchUtility(CMINVOKECOMMANDINFO* pici, IShellItemArray* psiItemArray);

    PCIDLIST_ABSOLUTE m_pidlFolder;
    IDataObject*      m_pdtobj;
    HBITMAP           m_hbmpIcon = nullptr;
    std::wstring      context_menu_caption;
};

OBJECT_ENTRY_AUTO(__uuidof(DCContextMenuHandler), CDCContextMenuHandler)
