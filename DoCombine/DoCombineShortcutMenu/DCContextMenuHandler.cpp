// DCContextMenuHandler.cpp : Implementation of CDCContextMenuHandler

#include "pch.h"
#include "resource.h"
#include "paths.h"
#include "logging.h"
#include <ShlObj_core.h>
#include <shellapi.h>
#include <wincodec.h>

#include <format>
#include <vector>
#include <gsl/gsl>

#include "DCContextMenuHandler.h"

namespace
{
// Lifted from powertoys/src/common/Themes/icon_helpers.cpp
HBITMAP create_bitmap_from_icon(_In_ HICON hIcon, _In_opt_ UINT width = 0, _In_opt_ UINT height = 0)
{
    HBITMAP hBitmapResult = NULL;

    // Create compatible DC
    HDC hDC = CreateCompatibleDC(NULL);
    if (hDC != NULL)
    {
        // Get bitmap rectangle size
        RECT rc   = {0};
        rc.left   = 0;
        rc.right  = (width != 0) ? width : GetSystemMetrics(SM_CXSMICON);
        rc.top    = 0;
        rc.bottom = (height != 0) ? height : GetSystemMetrics(SM_CYSMICON);

        // Create bitmap compatible with DC
        BITMAPINFO BitmapInfo;
        ZeroMemory(&BitmapInfo, sizeof(BITMAPINFO));

        BitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        BitmapInfo.bmiHeader.biWidth       = rc.right;
        BitmapInfo.bmiHeader.biHeight      = rc.bottom;
        BitmapInfo.bmiHeader.biPlanes      = 1;
        BitmapInfo.bmiHeader.biBitCount    = 32;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;

        HDC hDCBitmap = GetDC(NULL);

        HBITMAP hBitmap = CreateDIBSection(hDCBitmap, &BitmapInfo, DIB_RGB_COLORS, NULL, NULL, 0);

        ReleaseDC(NULL, hDCBitmap);

        if (hBitmap != NULL)
        {
            // Select bitmap into DC
            HBITMAP hBitmapOld = static_cast<HBITMAP>(SelectObject(hDC, hBitmap));
            if (hBitmapOld != NULL)
            {
                // Draw icon into DC
                if (DrawIconEx(hDC, 0, 0, hIcon, rc.right, rc.bottom, 0, NULL, DI_NORMAL))
                {
                    // Restore original bitmap in DC
                    hBitmapResult = static_cast<HBITMAP>(SelectObject(hDC, hBitmapOld));
                    hBitmapOld    = NULL;
                    hBitmap       = NULL;
                }

                if (hBitmapOld != NULL)
                {
                    SelectObject(hDC, hBitmapOld);
                }
            }

            if (hBitmap != NULL)
            {
                DeleteObject(hBitmap);
            }
        }

        DeleteDC(hDC);
    }

    return hBitmapResult;
}

HRESULT get_files_from_pdtobj(IDataObject* pdtobj, std::vector<std::wstring>& rdocs)
{
    FORMATETC formatetc   = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium   = {};
    auto volatile cleanup = gsl::finally([&stgmedium] {
        ReleaseStgMedium(&stgmedium);
    });

    if (FAILED(pdtobj->GetData(&formatetc, &stgmedium)))
    {
        return E_FAIL;
    }

    UINT itemCount = DragQueryFile(static_cast<HDROP>(stgmedium.hGlobal), 0xFFFFFFFF, NULL, 0);
    for (UINT i = 0; i < itemCount; ++i)
    {
        UINT   cch     = DragQueryFile(static_cast<HDROP>(stgmedium.hGlobal), i, NULL, 0) + 1;
        LPTSTR pszPath = static_cast<LPTSTR>(malloc(sizeof(TCHAR) * cch));
        DragQueryFile(static_cast<HDROP>(stgmedium.hGlobal), i, pszPath, cch);

        if (pszPath == NULL)
        {
            free(pszPath);
            continue;
        }

        LPWSTR pszExt = PathFindExtension(pszPath);

        if (pszExt != nullptr)
        {
            auto wsExt = std::wstring(pszExt);
            std::ranges::transform(wsExt, wsExt.begin(), ::towlower);

            if (!wsExt.empty())
            {
                rdocs.push_back(std::format(L"\"{}\"", pszPath));
            }
        }

        free(pszPath);
    }

    return S_OK;
}
} // namespace

extern HINSTANCE g_hInst_doCombineExt;

// CDCContextMenuHandler

CDCContextMenuHandler::CDCContextMenuHandler()
    : m_pidlFolder(nullptr), m_pdtobj(nullptr), context_menu_caption(L"Modify with DoCombine")
{
}

CDCContextMenuHandler::~CDCContextMenuHandler()
{
    Uninitialize();
}

void CDCContextMenuHandler::Uninitialize()
{
    ENTERED();
    CoTaskMemFree((LPVOID) m_pidlFolder);
    m_pidlFolder = NULL;

    if (m_pdtobj)
    {
        m_pdtobj->Release();
        m_pdtobj = NULL;
    }
    FINISHED();
}

// IShellExtInit
STDMETHODIMP CDCContextMenuHandler::Initialize(_In_opt_ PCIDLIST_ABSOLUTE pidlFolder, _In_opt_ IDataObject* pdtobj,
                                               _In_opt_ HKEY hkeyProgID)
{
    ENTERED();

    Uninitialize();
    if (pidlFolder)
    {
        m_pidlFolder = ILClone(pidlFolder);
    }

    if (pdtobj)
    {
        m_pdtobj = pdtobj;
        m_pdtobj->AddRef();
    }

    FINISHED(std::format("Returning {}", S_OK));
    return S_OK;
}

// IContextMenu
STDMETHODIMP CDCContextMenuHandler::QueryContextMenu(_In_ HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast,
                                                     UINT uFlags)
{
    ENTERED();
    if (uFlags & CMF_DEFAULTONLY)
    {
        FINISHED("Default only flag set");
        return S_OK;
    }

    // Iterate through the items in m_pdtobj
    std::vector<std::wstring> documents;
    if (FAILED(get_files_from_pdtobj(m_pdtobj, documents)))
    {
        FINISHED("Failed to get files from pdtobj");
        return E_FAIL;
    }
    // If there are any PDF files, create a menu item (mii)
    if (documents.empty())
    {
        FINISHED("No PDF documents found");
        return E_FAIL;
    }

    wchar_t strModifyDocs[128];
    wcscpy_s(strModifyDocs, ARRAYSIZE(strModifyDocs), context_menu_caption.c_str());

    MENUITEMINFO mii = {0};
    mii.cbSize       = sizeof(MENUITEMINFO);
    mii.fMask        = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
    mii.wID          = idCmdFirst;
    mii.fType        = MFT_STRING;
    mii.dwTypeData   = (LPWSTR) strModifyDocs;
    mii.fState       = MFS_ENABLED;
    HICON hIcon      = (HICON) LoadImage(g_hInst_doCombineExt, MAKEINTRESOURCE(IDI_DOCOMBINE), IMAGE_ICON, 16, 16, 0);
    if (hIcon)
    {
        mii.fMask |= MIIM_BITMAP;
        if (m_hbmpIcon == NULL)
        {
            m_hbmpIcon = create_bitmap_from_icon(hIcon);
        }
        mii.hbmpItem = m_hbmpIcon;
        DestroyIcon(hIcon);
    }

    // and call InsertMenuItem(hmenu, indexMenu, TRUE, &mii)
    HRESULT hr = 0;
    if (!InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1);
    }

    FINISHED(std::format("Returning: {}", hr));
    return hr;
}

STDMETHODIMP CDCContextMenuHandler::GetCommandString(UINT_PTR idCmd, UINT uType, _In_ UINT* pReserved, LPSTR pszName,
                                                     UINT cchMax)
{
    if (idCmd == ID_MODIFY_DOCS)
    {
        if (uType == GCS_VERBW)
        {
            wcscpy_s(reinterpret_cast<LPWSTR>(pszName), cchMax, MODIFY_DOCS_VERBW);
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

STDMETHODIMP CDCContextMenuHandler::InvokeCommand(_In_ CMINVOKECOMMANDINFO* pici)
{
    ENTERED();
    BOOL    fUnicode = FALSE;
    HRESULT hr       = E_FAIL;

    if (pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX) && pici->fMask & CMIC_MASK_UNICODE)
    {
        fUnicode = TRUE;
    }

    LOGGER.debug("fUnicode: {}, pici->lpVerb: {}", fUnicode, pici->lpVerb);
    if (!fUnicode && HIWORD(pici->lpVerb))
    {
    }
    else if (fUnicode && HIWORD(((CMINVOKECOMMANDINFOEX*) pici)->lpVerbW))
    {
        if (wcscmp((reinterpret_cast<CMINVOKECOMMANDINFOEX*>(pici))->lpVerbW, MODIFY_DOCS_VERBW) == 0)
        {
            hr = LaunchUtility(pici, nullptr);
        }
    }
    else if (LOWORD(pici->lpVerb) == 0)
    {
        hr = LaunchUtility(pici, nullptr);
    }

    FINISHED(std::format("Returning: {}", hr));
    return hr;
}

// IExplorerCommand
STDMETHODIMP CDCContextMenuHandler::GetTitle(IShellItemArray* /* psiItemArray */, LPWSTR* ppszName)
{
    return SHStrDup(context_menu_caption.c_str(), ppszName);
}

STDMETHODIMP CDCContextMenuHandler::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    std::wstring iconResourcePath = Paths::get_module_filenamew(g_hInst_doCombineExt);
    iconResourcePath += L",-";
    iconResourcePath += std::to_wstring(IDI_DOCOMBINE);
    return SHStrDup(iconResourcePath.c_str(), ppszIcon);
}

STDMETHODIMP CDCContextMenuHandler::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip)
{
    *ppszInfotip = nullptr;
    return E_NOTIMPL;
}

STDMETHODIMP CDCContextMenuHandler::GetCanonicalName(GUID* pguidCommandName)
{
    *pguidCommandName = __uuidof(this);
    return S_OK;
}

STDMETHODIMP CDCContextMenuHandler::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    ENTERED();

    // Hide if there is no PDF document
    *pCmdState = ECS_HIDDEN;
    // Suppressing C26812 warning as the issue is in the shtypes.h library
    // #pragma warning(suppress : 26812)
    // Check extension of first item in the list (the item which is right-clicked on)
    DWORD itemCount = 0;
    if (FAILED(psiItemArray->GetCount(&itemCount)))
    {
        FINISHED("Failed to get count of items in psiItemArray");
        return E_FAIL;
    }

    bool        foundPDF  = false;
    IShellItem* shellItem = nullptr;
    for (DWORD i = 0; i < itemCount; ++i)
    {
        if (FAILED(psiItemArray->GetItemAt(i, &shellItem)))
        {
            FINISHED("Failed to item in psiItemArray at position", i);
            return E_FAIL;
        }

        LPTSTR pszPath;
        // Retrieves the entire file system path of the file from its shell item
        HRESULT getDisplayResult = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
        if (S_OK != getDisplayResult || nullptr == pszPath)
        {
            // Avoid crashes in the following code.
            FINISHED("Failed to get file path of item at position", i);
            return E_FAIL;
        }

        LPTSTR pszExt = PathFindExtension(pszPath);
        if (nullptr == pszExt)
        {
            // Avoid crashes in the following code.
            FINISHED("Failed to get extension of item");
            CoTaskMemFree(pszPath);
            return E_FAIL;
        }

        // If selected file is a document...
        // TODO: If we decide to add support for adding images as PDF pages, this should
        // accept images as well
        if (pszExt != nullptr)
        {
            auto wsExt = std::wstring(pszExt);
            std::ranges::transform(wsExt, wsExt.begin(), ::towlower);
            if (!wsExt.empty() && wsExt == L"pdf")
            {
                foundPDF = true;
            }
        }
        CoTaskMemFree(pszPath);
    }

    if (foundPDF)
    {
        *pCmdState = ECS_ENABLED;
    }

    return S_OK;
}

STDMETHODIMP CDCContextMenuHandler::GetFlags(EXPCMDFLAGS* pFlags)
{
    *pFlags = ECF_DEFAULT;
    return S_OK;
}

STDMETHODIMP CDCContextMenuHandler::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    *ppEnum = nullptr;
    return E_NOTIMPL;
}

STDMETHODIMP CDCContextMenuHandler::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
{
    ENTERED("Launching utility");
    FINISHED();
    return LaunchUtility(nullptr, psiItemArray);
}

// Launching the main exe
HRESULT CDCContextMenuHandler::LaunchUtility(CMINVOKECOMMANDINFO* pici, IShellItemArray* psiItemArray)
{
    ENTERED();

    std::vector<std::wstring> documents;

    if (psiItemArray)
    {
        DWORD itemCount = 0;
        if (FAILED(psiItemArray->GetCount(&itemCount)))
        {
            LOGGER.error("Failed to get count of items in psiItemArray");
            return E_FAIL;
        }

        for (DWORD i = 0; i < itemCount; ++i)
        {
            IShellItem* shellItem = nullptr;
            if (FAILED(psiItemArray->GetItemAt(i, &shellItem)))
            {
                LOGGER.error("Failed to item in psiItemArray at position {}", i);
                return E_FAIL;
            }

            LPTSTR pszPath;
            // Retrieves the entire file system path of the file from its shell item
            HRESULT getDisplayResult = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
            if (S_OK != getDisplayResult || nullptr == pszPath)
            {
                // Avoid crashes in the following code.
                LOGGER.error("Failed to get file path of item at position {}", i);
                return E_FAIL;
            }

            LPTSTR pszExt = PathFindExtension(pszPath);
            if (pszExt != nullptr)
            {
                auto wsExt = std::wstring(pszExt);
                std::ranges::transform(wsExt, wsExt.begin(), ::towlower);

                if (!wsExt.empty() && wsExt == L"pdf")
                {
                    documents.push_back(std::format(L"\"{}\"", pszPath));
                }
            }
            else
            {
                LOGGER.error("Failed to get extension for file");
                CoTaskMemFree(pszPath);
                return E_FAIL;
            }
            CoTaskMemFree(pszPath);
        }
    }
    else
    {
        if (FAILED(get_files_from_pdtobj(m_pdtobj, documents)))
        {
            return E_FAIL;
        }
    }

    std::wstring utilityPath =
        std::format(L"{}\\..\\DoCombine.exe", Paths::get_module_folderpathw(g_hInst_doCombineExt));

    LOGGER.debug("Path to executable is: {}",
                 std::format("{}\\..\\DoCombine.exe", Paths::get_module_folderpatha(g_hInst_doCombineExt)));

    for (const auto& doc : documents)
    {
        utilityPath += L" " + doc;
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb          = sizeof(STARTUPINFO);
    si.dwFlags     = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    PROCESS_INFORMATION procInfo;
    if (!CreateProcess(NULL, utilityPath.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &procInfo))
    {
        LOGGER.error("Failed to launch utility. Last error: {}", GetLastError());
        return E_FAIL;
    }

    if (!CloseHandle(procInfo.hProcess))
    {
        auto hr = HRESULT_FROM_WIN32(GetLastError());
        LOGGER.error("Failed to launch utility. Last error: {}", hr);
        return hr;
    }
    if (!CloseHandle(procInfo.hThread))
    {
        auto hr = HRESULT_FROM_WIN32(GetLastError());
        LOGGER.error("Failed to launch utility. Last error: {}", hr);
        return hr;
    }

    return S_OK;
}
