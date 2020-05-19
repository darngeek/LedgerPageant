#pragma once

#include <vector>
#include <string>
#include <tchar.h>
#include "logger.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

class RegistryInterface {
public:
    std::vector<Identity> ReadIdentities(HKEY hKey)
    {
        std::vector<Identity> idents;

        TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
        DWORD    cbName;                   // size of name string 
        TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
        DWORD    cchClassName = MAX_PATH;  // size of class string 
        DWORD    cSubKeys = 0;               // number of subkeys 
        DWORD    cbMaxSubKey;              // longest subkey size 
        DWORD    cchMaxClass;              // longest class string 
        DWORD    cValues;              // number of values for key 
        DWORD    cchMaxValue;          // longest value name 
        DWORD    cbMaxValueData;       // longest value data 
        DWORD    cbSecurityDescriptor; // size of security descriptor 
        FILETIME ftLastWriteTime;      // last write time 
        DWORD i, retCode;
        DWORD cchValue = MAX_VALUE_NAME;

        // Get the class name and the value count. 
        retCode = RegQueryInfoKey(
            hKey,                    // key handle 
            achClass,                // buffer for class name 
            &cchClassName,           // size of class string 
            NULL,                    // reserved 
            &cSubKeys,               // number of subkeys 
            &cbMaxSubKey,            // longest subkey size 
            &cchMaxClass,            // longest class string 
            &cValues,                // number of values for this key 
            &cchMaxValue,            // longest value name 
            &cbMaxValueData,         // longest value data 
            &cbSecurityDescriptor,   // security descriptor 
            &ftLastWriteTime);       // last write time 

        // Enumerate the subkeys, until RegEnumKeyEx fails.

        if (cSubKeys)
        {
            printf("\nNumber of subkeys: %d\n", cSubKeys);

            for (i = 0; i < cSubKeys; i++)
            {
                cbName = MAX_KEY_LENGTH;
                retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL, &ftLastWriteTime);
                if (retCode == ERROR_SUCCESS) {
                    _tprintf(TEXT("(%d) %s\n"), i + 1, achKey);

                    // get the following key-values:
                    Identity ident;
                    ident.name = achKey;
                    ident.user = getValueWStringFor(hKey, achKey, _T("UserName"));
                    ident.host = getValueWStringFor(hKey, achKey, _T("HostName"));
                    ident.protocol = getValueWStringFor(hKey, achKey, _T("Protocol"));
                    ident.port = getValueDwordFor(hKey, achKey, _T("PortNumber"));

                    // store key
                    _tcscpy_s(ident.regKeyName, 255, achKey);
                   
                    idents.push_back(ident);
                }
            }
        }

        return idents;
    }

    std::wstring getValueWStringFor(HKEY hKey, TCHAR* achKey, const std::wstring& value) {
        // TODO: move to more global?
        TCHAR retValue[MAX_VALUE_NAME];
        DWORD retLength = MAX_VALUE_NAME;

        LONG retCode = ::RegGetValue(hKey, achKey, value.c_str(), RRF_RT_REG_SZ, nullptr, retValue, &retLength);
        
        if (retCode == ERROR_SUCCESS) {
            std::wstring wide(retValue);
            return wide;
        }

        return std::wstring();
    }

    int getValueDwordFor(HKEY hKey, TCHAR* achKey, const std::wstring& value) {
        DWORD retValue;

        LONG retCode = ::RegGetValue(hKey, achKey, value.c_str(), RRF_RT_DWORD, nullptr, &retValue, NULL);
        if (retCode == ERROR_SUCCESS) {
            return retValue;
        }

        return -1;
    }

    std::vector<Identity> getSessions() {
        const std::wstring& subKey = _T("Software\\SimonTatham\\PuTTy\\Sessions\\");

        std::vector<Identity> sessions;
        HKEY hTestKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hTestKey) == ERROR_SUCCESS) {
            sessions = ReadIdentities(hTestKey);
        }
        RegCloseKey(hTestKey);

        return sessions;
    }

    bool RemoveKeyRecursive(std::wstring inKey) {
        std::wstring subkeyStr(inKey);
        LONG lResult;
        DWORD dwSize;
        TCHAR szName[MAX_PATH];
        HKEY hKey;
        FILETIME ftWrite;

        // First, see if we can delete the key without recurse
        lResult = RegDeleteKey(HKEY_CURRENT_USER, subkeyStr.c_str());
        if (lResult == ERROR_SUCCESS) {
            return true;
        }

        lResult = RegOpenKeyEx(HKEY_CURRENT_USER, subkeyStr.c_str(), 0, KEY_READ, &hKey);
        if (lResult != ERROR_SUCCESS) {
            if (lResult == ERROR_FILE_NOT_FOUND) {
                return true;
            }
            else {
                LPCWSTR title = L"Error opening key";
                LPCWSTR description = L"Error opening registry key for identity";

                int msgboxID = MessageBox(NULL, description, title, MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
                LOG_WARN("Error opening key");
                return false;
            }
        }

        // Check for an ending slash and add one if it is missing.
        if (subkeyStr[subkeyStr.size() - 1] != TEXT('\\')) {
            subkeyStr += TEXT('\\');
            subkeyStr += TEXT('\0');
        }

        // Enumerate the keys
        dwSize = MAX_PATH;
        lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);
        if (lResult == ERROR_SUCCESS) {
            do {
                subkeyStr[subkeyStr.size()] = TEXT('\0');
                subkeyStr += szName;

                if (!RemoveKeyRecursive(subkeyStr)) {
                    break;
                }

                dwSize = MAX_PATH;

                lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);
            } while (lResult == ERROR_SUCCESS);
        }

        subkeyStr[subkeyStr.size() - 1] = TEXT('\0');

        RegCloseKey(hKey);

        // Try again to delete the key.
        lResult = RegDeleteKey(HKEY_CURRENT_USER, subkeyStr.c_str());
        if (lResult == ERROR_SUCCESS) {
            return true;
        }

        return false;
    }

    bool removeSession(Identity ident) {
        std::wstring subkeyStr(_T("Software\\SimonTatham\\PuTTy\\Sessions\\"));
        subkeyStr += ident.regKeyName;
        return RemoveKeyRecursive(subkeyStr);
    }

    bool SetStringValueForKey(HKEY& hKey, LPCTSTR value, LPCTSTR data) {
        LONG setRes = RegSetValueEx(hKey, value, 0, REG_SZ, (LPBYTE)data, (_tcslen(data) + 1) * sizeof(WCHAR));
        if (setRes == ERROR_SUCCESS) {
            return true;
        }
        else {
            LOG_ERR("failed to write %s to Registry.", value);
            return false;
        }
    }

    bool SetDwordValueForKey(HKEY& hKey, LPCTSTR value, DWORD data) {
        LONG setRes = RegSetValueEx(hKey, value, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        return (setRes == ERROR_SUCCESS);
    }

    bool createSession(Identity ident) {
        HKEY hKey;

        std::wstring subkeyStr(_T("Software\\SimonTatham\\PuTTy\\Sessions\\"));
        subkeyStr += ident.name;
        subkeyStr += TEXT('\\');

        LONG openRes = RegCreateKeyEx(HKEY_CURRENT_USER, subkeyStr.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        if (openRes == ERROR_SUCCESS) {
            std::string identName(ident.name.begin(), ident.name.end());
            LOG_DBG("Loaded key for %s", identName.c_str());
        }
        else if (openRes == ERROR_ACCESS_DENIED) {
            LOG_ERR("Access denied");
            return false;
        }
        else {
            LOG_ERR("Error creating key.");
            return false;
        }

        SetStringValueForKey(hKey, TEXT("HostName"), ident.host.c_str());
        SetDwordValueForKey(hKey, TEXT("PortNumber"), (DWORD)ident.port);
        SetStringValueForKey(hKey, TEXT("UserName"), ident.user.c_str());
        SetStringValueForKey(hKey, TEXT("Protocol"), ident.protocol.c_str());

        LONG closeOut = RegCloseKey(hKey);

        return true;
    }
};