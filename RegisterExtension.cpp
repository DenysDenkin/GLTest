#include "RegisterExtension.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <shobjidl.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")     // link to this

__inline HRESULT ResultFromKnownLastError() { const DWORD err = GetLastError(); return err == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32(err); }

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
__inline HINSTANCE GetModuleHINSTANCE() { return (HINSTANCE)&__ImageBase; }

CRegisterExtension::CRegisterExtension(REFCLSID clsid /* = CLSID_NULL */, HKEY hkeyRoot /* = HKEY_CURRENT_USER */) : _hkeyRoot(hkeyRoot), _fAssocChanged(false)
{
    SetHandlerCLSID(clsid);
    GetModuleFileName(GetModuleHINSTANCE(), _szModule, ARRAYSIZE(_szModule));
}

CRegisterExtension::~CRegisterExtension()
{
    if (_fAssocChanged)
    {
        SHChangeNotify(SHCNE_ASSOCCHANGED, 0, 0, 0);
    }
}

void CRegisterExtension::SetHandlerCLSID(REFCLSID clsid)
{
    _clsid = clsid;
    StringFromGUID2(_clsid, _szCLSID, ARRAYSIZE(_szCLSID));
}

void CRegisterExtension::SetInstallScope(HKEY hkeyRoot)
{
    _hkeyRoot = hkeyRoot;
}

HRESULT CRegisterExtension::SetModule(PCWSTR pszModule)
{
    return StringCchCopy(_szModule, ARRAYSIZE(_szModule), pszModule);
}

HRESULT CRegisterExtension::SetModule(HINSTANCE hinst)
{
    return GetModuleFileName(hinst, _szModule, ARRAYSIZE(_szModule)) ? S_OK : E_FAIL;
}

HRESULT CRegisterExtension::_EnsureModule() const
{
    return _szModule[0] ? S_OK : E_FAIL;
}

HRESULT CRegisterExtension::RegisterAppAsLocalServer(PCWSTR pszFriendlyName, PCWSTR pszCmdLine) const
{
    HRESULT hr = _EnsureModule();
    if (SUCCEEDED(hr))
    {
        WCHAR szCmdLine[MAX_PATH + 20];
        if (pszCmdLine)
        {
            StringCchPrintf(szCmdLine, ARRAYSIZE(szCmdLine), L"%s %s", _szModule, pszCmdLine);
        }
        else
        {
            StringCchCopy(szCmdLine, ARRAYSIZE(szCmdLine), _szModule);
        }

        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s\\LocalServer32", L"", szCmdLine, _szCLSID);
        if (SUCCEEDED(hr))
        {
            hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s", L"AppId", _szCLSID, _szCLSID);
            if (SUCCEEDED(hr))
            {
                hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s", L"", pszFriendlyName, _szCLSID);
                if (SUCCEEDED(hr))
                {               
                    hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\AppID\\%s", L"", pszFriendlyName, _szCLSID);
                }
            }
        }
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterElevatableLocalServer(PCWSTR pszFriendlyName, UINT idLocalizeString, UINT idIconRef) const
{
    HRESULT hr = _EnsureModule();
    if (SUCCEEDED(hr))
    {
        hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s", L"", pszFriendlyName, _szCLSID);
        if (SUCCEEDED(hr))
        {
            WCHAR szRes[MAX_PATH + 20];
            StringCchPrintf(szRes, ARRAYSIZE(szRes), L"@%s,-%d", _szModule, idLocalizeString);
            hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s", L"LocalizedString", szRes, _szCLSID);
            if (SUCCEEDED(hr))
            {
                hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s\\LocalServer32", L"", _szModule, _szCLSID);
                if (SUCCEEDED(hr))
                {
                    hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s\\Elevation", L"Enabled", 1, _szCLSID);
                    if (SUCCEEDED(hr) && idIconRef)
                    {
                        StringCchPrintf(szRes, ARRAYSIZE(szRes), L"@%s,-%d", _szModule, idIconRef);
                        hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s\\Elevation", L"IconReference", szRes, _szCLSID);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterElevatableInProcServer(PCWSTR pszFriendlyName, UINT idLocalizeString, UINT idIconRef) const
{
    HRESULT hr = _EnsureModule();
    if (SUCCEEDED(hr))
    {
        hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\AppId\\%s", L"", pszFriendlyName, _szCLSID);
        if (SUCCEEDED(hr))
        {
            const unsigned char c_rgAccessPermission[] =
                {0x01,0x00,0x04,0x80,0x60,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,
                 0x00,0x00,0x00,0x02,0x00,0x4c,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x03,0x00,
                 0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x12,0x00,0x00,0x00,0x00,0x00,0x14,
                 0x00,0x07,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x0a,0x00,0x00,0x00,
                 0x00,0x00,0x14,0x00,0x03,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x04,
                 0x00,0x00,0x00,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0x01,0x02,0x00,0x00,0x00,0x00,
                 0x00,0x05,0x20,0x00,0x00,0x00,0x20,0x02,0x00,0x00,0x01,0x02,0x00,0x00,0x00,0x00,0x00,
                 0x05,0x20,0x00,0x00,0x00,0x20,0x02,0x00,0x00};
            hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\AppId\\%s", L"AccessPermission", c_rgAccessPermission, sizeof(c_rgAccessPermission), _szCLSID);

            const unsigned char c_rgLaunchPermission[] =
                {0x01,0x00,0x04,0x80,0x78,0x00,0x00,0x00,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,
                 0x00,0x00,0x00,0x02,0x00,0x64,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x1f,0x00,
                 0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x12,0x00,0x00,0x00,0x00,0x00,0x18,
                 0x00,0x1f,0x00,0x00,0x00,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x05,0x20,0x00,0x00,0x00,
                 0x20,0x02,0x00,0x00,0x00,0x00,0x14,0x00,0x1f,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,
                 0x00,0x00,0x05,0x04,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x0b,0x00,0x00,0x00,0x01,0x01,
                 0x00,0x00,0x00,0x00,0x00,0x05,0x12,0x00,0x00,0x00,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,
                 0xcd,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x05,0x20,0x00,0x00,0x00,0x20,0x02,0x00,0x00,
                 0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x05,0x20,0x00,0x00,0x00,0x20,0x02,0x00,0x00};
            hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\AppId\\%s", L"LaunchPermission", c_rgLaunchPermission, sizeof(c_rgLaunchPermission), _szCLSID);

            hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s", L"", pszFriendlyName, _szCLSID);
            if (SUCCEEDED(hr))
            {
                hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s", L"AppId", _szCLSID, _szCLSID);
                if (SUCCEEDED(hr))
                {
                    WCHAR szRes[MAX_PATH + 20];
                    StringCchPrintf(szRes, ARRAYSIZE(szRes), L"@%s,-%d", _szModule, idLocalizeString);
                    hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s", L"LocalizedString", szRes, _szCLSID);
                    if (SUCCEEDED(hr))
                    {
                        hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s\\InProcServer32", L"", _szModule, _szCLSID);
                        if (SUCCEEDED(hr))
                        {
                            hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s\\Elevation", L"Enabled", 1, _szCLSID);
                            if (SUCCEEDED(hr) && idIconRef)
                            {
                                StringCchPrintf(szRes, ARRAYSIZE(szRes), L"@%s,-%d", _szModule, idIconRef);
                                hr = RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\%s\\Elevation", L"IconReference", szRes, _szCLSID);
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterInProcServer(PCWSTR pszFriendlyName, PCWSTR pszThreadingModel) const
{
    HRESULT hr = _EnsureModule();
    if (SUCCEEDED(hr))
    {
        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s", L"", pszFriendlyName, _szCLSID);
        if (SUCCEEDED(hr))
        {
            hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s\\InProcServer32", L"", _szModule, _szCLSID);
            if (SUCCEEDED(hr))
            {
                hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s\\InProcServer32", L"ThreadingModel", pszThreadingModel, _szCLSID);
            }
        }
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterInProcServerAttribute(PCWSTR pszAttribute, DWORD dwValue) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s", pszAttribute, dwValue, _szCLSID);
}

HRESULT CRegisterExtension::UnRegisterObject() const
{
    HRESULT hr = RegDeleteKeyPrintf(_hkeyRoot, L"Software\\Classes\\AppID\\%s", _szCLSID);
    if (SUCCEEDED(hr))
    {
        hr = RegDeleteKeyPrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s", _szCLSID);
    }
    return hr;
}


HRESULT CRegisterExtension::RegisterHandlerSupportedProtocols(PCWSTR pszProtocol) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s\\SupportedProtocols", pszProtocol, L"", _szCLSID);
}


HRESULT CRegisterExtension::RegisterAppDropTarget() const
{
    HRESULT hr = _EnsureModule();
    if (SUCCEEDED(hr))
    {
        hr = RegSetKeyValuePrintf(_hkeyRoot,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s",
            L"DropTarget", _szCLSID, PathFindFileName(_szModule));
    }
    return hr;
}


bool CRegisterExtension::_IsBaseClassProgID(PCWSTR pszProgID) const
{
    return !StrCmpIC(pszProgID, L"AllFileSystemObjects") ||
           !StrCmpIC(pszProgID, L"Directory") ||
           !StrCmpIC(pszProgID, L"*") ||
           StrStrI(pszProgID, L"SystemFileAssociations\\Directory.");
}

HRESULT CRegisterExtension::_EnsureBaseProgIDVerbIsNone(PCWSTR pszProgID) const
{
    return _IsBaseClassProgID(pszProgID) ?
        RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell", L"", L"none", pszProgID) :
        S_OK;
}

HRESULT CRegisterExtension::RegisterCreateProcessVerb(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszCmdLine, PCWSTR pszVerbDisplayName) const
{
    UnRegisterVerb(pszProgID, pszVerb);

    HRESULT hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shell\\%s\\command", L"", pszCmdLine, pszProgID, pszVerb);
    if (SUCCEEDED(hr))
    {
        hr = _EnsureBaseProgIDVerbIsNone(pszProgID);

        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shell\\%s", L"", pszVerbDisplayName, pszProgID, pszVerb);
    }
    return hr;
}


HRESULT CRegisterExtension::RegisterDropTargetVerb(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszVerbDisplayName) const
{
    UnRegisterVerb(pszProgID, pszVerb);

    HRESULT hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s\\DropTarget",
        L"CLSID", _szCLSID, pszProgID, pszVerb);
    if (SUCCEEDED(hr))
    {
        hr = _EnsureBaseProgIDVerbIsNone(pszProgID);

        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s",
            L"", pszVerbDisplayName, pszProgID, pszVerb);
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterExecuteCommandVerb(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszVerbDisplayName) const
{
    UnRegisterVerb(pszProgID, pszVerb);

    HRESULT hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s\\command",
        L"DelegateExecute", _szCLSID, pszProgID, pszVerb);
    if (SUCCEEDED(hr))
    {
        hr = _EnsureBaseProgIDVerbIsNone(pszProgID);

        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s",
            L"", pszVerbDisplayName, pszProgID, pszVerb);
    }
    return hr;
}


HRESULT CRegisterExtension::RegisterExplorerCommandVerb(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszVerbDisplayName) const
{
    UnRegisterVerb(pszProgID, pszVerb);

    HRESULT hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s",
        L"ExplorerCommandHandler", _szCLSID, pszProgID, pszVerb);
    if (SUCCEEDED(hr))
    {
        hr = _EnsureBaseProgIDVerbIsNone(pszProgID);

        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s",
            L"", pszVerbDisplayName, pszProgID, pszVerb);
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterExplorerCommandStateHandler(PCWSTR pszProgID, PCWSTR pszVerb) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s",
        L"CommandStateHandler", _szCLSID, pszProgID, pszVerb);
}

HRESULT CRegisterExtension::UnRegisterVerb(PCWSTR pszProgID, PCWSTR pszVerb) const
{
    return RegDeleteKeyPrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell\\%s", pszProgID, pszVerb);
}

HRESULT CRegisterExtension::UnRegisterVerbs(PCWSTR const rgpszAssociation[], UINT countAssociation, PCWSTR pszVerb) const
{
    HRESULT hr = S_OK;
    for (UINT i = 0; SUCCEEDED(hr) && (i < countAssociation); i++)
    {
        hr = UnRegisterVerb(rgpszAssociation[i], pszVerb);
    }

    if (SUCCEEDED(hr) && HasClassID())
    {
        hr = UnRegisterObject();
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterThumbnailHandler(PCWSTR pszExtension) const
{

    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}",
        L"", _szCLSID, pszExtension);
}


HRESULT CRegisterExtension::RegisterRightDragContextMenuHandler(PCWSTR pszProgID, PCWSTR pszDescription) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shellex\\DragDropHandlers\\%s",
        L"", pszDescription, pszProgID, _szCLSID);
}


HRESULT CRegisterExtension::RegisterContextMenuHandler(PCWSTR pszProgID, PCWSTR pszDescription) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shellex\\ContextMenuHandlers\\%s",
        L"", pszDescription, pszProgID, _szCLSID);
}

HRESULT CRegisterExtension::RegisterPropertyHandler(PCWSTR pszExtension) const
{

    return RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\%s",
        L"", _szCLSID, pszExtension);
}

HRESULT CRegisterExtension::UnRegisterPropertyHandler(PCWSTR pszExtension) const
{
    return RegDeleteKeyPrintf(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\%s", pszExtension);
}

HRESULT CRegisterExtension::RegisterLinkHandler(PCWSTR pszProgID) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\ShellEx\\LinkHandler", L"", _szCLSID, pszProgID);
}


HRESULT CRegisterExtension::RegisterProgID(PCWSTR pszProgID, PCWSTR pszTypeName, UINT idIcon) const
{
    HRESULT hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s", L"", pszTypeName, pszProgID);
    if (SUCCEEDED(hr))
    {
        if (idIcon)
        {
            WCHAR szIconRef[MAX_PATH];
            StringCchPrintf(szIconRef, ARRAYSIZE(szIconRef), L"\"%s\",-%d", _szModule, idIcon);
            hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\DefaultIcon", L"", szIconRef, pszProgID);
        }
    }
    return hr;
}

HRESULT CRegisterExtension::UnRegisterProgID(PCWSTR pszProgID, PCWSTR pszFileExtension) const
{
    HRESULT hr = RegDeleteKeyPrintf(_hkeyRoot, L"Software\\Classes\\%s", pszProgID);
    if (SUCCEEDED(hr) && pszFileExtension)
    {
        hr = RegDeleteKeyPrintf(_hkeyRoot, L"Software\\Classes\\%s\\%s", pszFileExtension, pszProgID);
    }
    return hr;
}


HRESULT CRegisterExtension::RegisterProgIDValue(PCWSTR pszProgID, PCWSTR pszValueName) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s", pszValueName, L"", pszProgID);
}


HRESULT CRegisterExtension::RegisterProgIDValue(PCWSTR pszProgID, PCWSTR pszValueName, PCWSTR pszValue) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s", pszValueName, pszValue, pszProgID);
}


HRESULT CRegisterExtension::RegisterProgIDValue(PCWSTR pszProgID, PCWSTR pszValueName, DWORD dwValue) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s", pszValueName, dwValue, pszProgID);
}

HRESULT CRegisterExtension::RegisterVerbAttribute(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszValueName) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shell\\%s", pszValueName, L"", pszProgID, pszVerb);
}

HRESULT CRegisterExtension::RegisterVerbAttribute(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszValueName, PCWSTR pszValue) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shell\\%s", pszValueName, pszValue, pszProgID, pszVerb);
}

HRESULT CRegisterExtension::RegisterVerbAttribute(PCWSTR pszProgID, PCWSTR pszVerb, PCWSTR pszValueName, DWORD dwValue) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\shell\\%s", pszValueName, dwValue, pszProgID, pszVerb);
}

HRESULT CRegisterExtension::RegisterVerbDefaultAndOrder(PCWSTR pszProgID, PCWSTR pszVerbOrderFirstIsDefault) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\Shell", L"", pszVerbOrderFirstIsDefault, pszProgID);
}


HRESULT CRegisterExtension::RegisterPlayerVerbs(PCWSTR const rgpszAssociation[], UINT countAssociation,
                                                PCWSTR pszVerb, PCWSTR pszTitle) const
{
    HRESULT hr = RegisterAppAsLocalServer(pszTitle, NULL);
    if (SUCCEEDED(hr))
    {
        hr = RegisterHandlerSupportedProtocols(L"*");

        for (UINT i = 0; SUCCEEDED(hr) && (i < countAssociation); i++)
        {
            hr = RegisterExecuteCommandVerb(rgpszAssociation[i], pszVerb, pszTitle);
            if (SUCCEEDED(hr))
            {
                hr = RegisterVerbAttribute(rgpszAssociation[i], pszVerb, L"NeverDefault");
                if (SUCCEEDED(hr))
                {
                    hr = RegisterVerbAttribute(rgpszAssociation[i], pszVerb, L"MultiSelectModel", L"Player");
                }
            }
        }
    }
    return hr;
}


HRESULT CRegisterExtension::RegisterExtensionWithProgID(PCWSTR pszFileExtension, PCWSTR pszProgID) const
{

    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s", L"", pszProgID, pszFileExtension);
}


HRESULT CRegisterExtension::RegisterOpenWith(PCWSTR pszFileExtension, PCWSTR pszProgID) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\OpenWithProgIds", pszProgID, L"", pszFileExtension);
}

HRESULT CRegisterExtension::RegisterNewMenuNullFile(PCWSTR pszFileExtension, PCWSTR pszProgID) const
{

    HRESULT hr;
    if (pszProgID)
    {
        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\%s\\ShellNew", L"NullFile", L"", pszFileExtension, pszProgID);
    }
    else
    {
        hr = RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\%s\\ShellNew", L"NullFile", L"", pszFileExtension);
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterNewMenuData(PCWSTR pszFileExtension, PCWSTR pszProgID, PCSTR pszBase64) const
{
    HRESULT hr;
    if (pszProgID)
    {
        hr = RegSetKeyValueBinaryPrintf(_hkeyRoot, L"Software\\Classes\\%s\\%s\\ShellNew", L"Data", pszBase64, pszFileExtension, pszProgID);
    }
    else
    {
        hr = RegSetKeyValueBinaryPrintf(_hkeyRoot, L"Software\\Classes\\%s\\ShellNew", L"Data", pszBase64, pszFileExtension);
    }
    return hr;
}

HRESULT CRegisterExtension::RegisterKind(PCWSTR pszFileExtension, PCWSTR pszKindValue) const
{
    return RegSetKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\KindMap", pszFileExtension, pszKindValue);
}

HRESULT CRegisterExtension::UnRegisterKind(PCWSTR pszFileExtension) const
{
    return RegDeleteKeyValuePrintf(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\KindMap", pszFileExtension);
}


HRESULT CRegisterExtension::RegisterPropertyHandlerOverride(PCWSTR pszProperty) const
{
    return RegSetKeyValuePrintf(_hkeyRoot, L"Software\\Classes\\CLSID\\%s\\OverrideFileSystemProperties", pszProperty, 1, _szCLSID);
}

HRESULT CRegisterExtension::RegisterAppShortcutInSendTo() const
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) ? S_OK : ResultFromKnownLastError();
    if (SUCCEEDED(hr))
    {
        IShellLink *psl;
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
        if (SUCCEEDED(hr))
        {
            hr = psl->SetPath(szPath);
            if (SUCCEEDED(hr))
            {
                WCHAR szName[MAX_PATH];
                StringCchCopy(szName, ARRAYSIZE(szName), PathFindFileName(szPath));
                PathRenameExtension(szName, L".lnk");

                hr = SHGetFolderPath(NULL, CSIDL_SENDTO, NULL, 0, szPath);
                if (SUCCEEDED(hr))
                {
                    hr = PathAppend(szPath, szName) ? S_OK : E_FAIL;
                    if (SUCCEEDED(hr))
                    {
                        IPersistFile *ppf;
                        hr = psl->QueryInterface(IID_PPV_ARGS(&ppf));
                        if (SUCCEEDED(hr))
                        {
                            hr = ppf->Save(szPath, TRUE);
                            ppf->Release();
                        }
                    }
                }
            }
            psl->Release();
        }
    }
    return hr;
}

HRESULT CRegisterExtension::RegSetKeyValuePrintf(HKEY hkey, PCWSTR pszKeyFormatString, PCWSTR pszValueName, PCWSTR pszValue, ...) const
{
    va_list argList;
    va_start(argList, pszValue);

    WCHAR szKeyName[512];
    HRESULT hr = StringCchVPrintf(szKeyName, ARRAYSIZE(szKeyName), pszKeyFormatString, argList);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetKeyValueW(hkey, szKeyName, pszValueName, REG_SZ, pszValue,
            lstrlen(pszValue) * sizeof(*pszValue)));
    }

    va_end(argList);

    _UpdateAssocChanged(hr, pszKeyFormatString);
    return hr;
}

HRESULT CRegisterExtension::RegSetKeyValuePrintf(HKEY hkey, PCWSTR pszKeyFormatString, PCWSTR pszValueName, DWORD dwValue, ...) const
{
    va_list argList;
    va_start(argList, dwValue);

    WCHAR szKeyName[512];
    HRESULT hr = StringCchVPrintf(szKeyName, ARRAYSIZE(szKeyName), pszKeyFormatString, argList);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetKeyValueW(hkey, szKeyName, pszValueName, REG_DWORD, &dwValue, sizeof(dwValue)));
    }

    va_end(argList);

    _UpdateAssocChanged(hr, pszKeyFormatString);
    return hr;
}

HRESULT CRegisterExtension::RegSetKeyValuePrintf(HKEY hkey, PCWSTR pszKeyFormatString, PCWSTR pszValueName, const unsigned char pc[], DWORD dwSize, ...) const
{
    va_list argList;
    va_start(argList, pc);

    WCHAR szKeyName[512];
    HRESULT hr = StringCchVPrintf(szKeyName, ARRAYSIZE(szKeyName), pszKeyFormatString, argList);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetKeyValueW(hkey, szKeyName, pszValueName, REG_BINARY, pc, dwSize));
    }

    va_end(argList);

    _UpdateAssocChanged(hr, pszKeyFormatString);
    return hr;
}

HRESULT CRegisterExtension::RegSetKeyValueBinaryPrintf(HKEY hkey, PCWSTR pszKeyFormatString, PCWSTR pszValueName, PCSTR pszBase64, ...) const
{
    va_list argList;
    va_start(argList, pszBase64);

    WCHAR szKeyName[512];
    HRESULT hr = StringCchVPrintf(szKeyName, ARRAYSIZE(szKeyName), pszKeyFormatString, argList);
    if (SUCCEEDED(hr))
    {
        DWORD dwDecodedImageSize, dwSkipChars, dwActualFormat;
        hr = CryptStringToBinaryA(pszBase64, NULL, CRYPT_STRING_BASE64, NULL,
            &dwDecodedImageSize, &dwSkipChars, &dwActualFormat) ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            BYTE *pbDecodedImage = (BYTE*)LocalAlloc(LPTR, dwDecodedImageSize);
            hr = pbDecodedImage ? S_OK : E_OUTOFMEMORY;
            if (SUCCEEDED(hr))
            {
                hr = CryptStringToBinaryA(pszBase64, lstrlenA(pszBase64), CRYPT_STRING_BASE64,
                    pbDecodedImage, &dwDecodedImageSize, &dwSkipChars, &dwActualFormat) ? S_OK : E_FAIL;
                if (SUCCEEDED(hr))
                {
                    hr = HRESULT_FROM_WIN32(RegSetKeyValueW(hkey, szKeyName, pszValueName, REG_BINARY, pbDecodedImage, dwDecodedImageSize));
                }
            }
        }
    }

    va_end(argList);

    _UpdateAssocChanged(hr, pszKeyFormatString);
    return hr;
}

__inline HRESULT MapNotFoundToSuccess(HRESULT hr)
{
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ? S_OK : hr;
}

HRESULT CRegisterExtension::RegDeleteKeyPrintf(HKEY hkey, PCWSTR pszKeyFormatString, ...) const
{
    va_list argList;
    va_start(argList, pszKeyFormatString);

    WCHAR szKeyName[512];
    HRESULT hr = StringCchVPrintf(szKeyName, ARRAYSIZE(szKeyName), pszKeyFormatString, argList);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegDeleteTree(hkey, szKeyName));
    }

    va_end(argList);

    _UpdateAssocChanged(hr, pszKeyFormatString);
    return MapNotFoundToSuccess(hr);
}

HRESULT CRegisterExtension::RegDeleteKeyValuePrintf(HKEY hkey, PCWSTR pszKeyFormatString, PCWSTR pszValue, ...) const
{
    va_list argList;
    va_start(argList, pszKeyFormatString);

    WCHAR szKeyName[512];
    HRESULT hr = StringCchVPrintf(szKeyName, ARRAYSIZE(szKeyName), pszKeyFormatString, argList);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegDeleteKeyValueW(hkey, szKeyName, pszValue));
    }

    va_end(argList);

    _UpdateAssocChanged(hr, pszKeyFormatString);
    return MapNotFoundToSuccess(hr);
}

void CRegisterExtension::_UpdateAssocChanged(HRESULT hr, PCWSTR pszKeyFormatString) const
{
    static const WCHAR c_szProgIDPrefix[] = L"Software\\Classes\\%s";
    if (SUCCEEDED(hr) && !_fAssocChanged &&
        (StrCmpNIC(pszKeyFormatString, c_szProgIDPrefix, ARRAYSIZE(c_szProgIDPrefix) - 1) == 0 ||
         StrStrI(pszKeyFormatString, L"PropertyHandlers") ||
         StrStrI(pszKeyFormatString, L"KindMap")))
    {
        const_cast<CRegisterExtension*>(this)->_fAssocChanged = true;
    }
}
