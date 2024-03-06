#pragma once

#include "pch.h"
#include <string>

namespace Paths
{
// Lifted from powertoys/src/common/utils/process_path.h
inline std::wstring get_module_filenamew(HMODULE mod = nullptr)
{
    wchar_t buffer[MAX_PATH + 1];
    DWORD   actual_length = GetModuleFileNameW(mod, buffer, MAX_PATH);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        const DWORD  long_path_length = 0xFFFF; // should be always enough
        std::wstring long_filename(long_path_length, L'\0');
        actual_length = GetModuleFileNameW(mod, (LPWSTR) long_filename.c_str(), long_path_length);
        return long_filename.substr(0, actual_length);
    }
    return {buffer, actual_length};
}

inline std::string get_module_filenamea(HMODULE mod = nullptr)
{
    char  buffer[MAX_PATH + 1];
    DWORD actual_length = GetModuleFileNameA(mod, buffer, MAX_PATH);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        const DWORD long_path_length = 0xFFFF; // should be always enough
        std::string long_filename(long_path_length, L'\0');
        actual_length = GetModuleFileNameA(mod, (LPSTR) long_filename.c_str(), long_path_length);
        return long_filename.substr(0, actual_length);
    }
    return {buffer, actual_length};
}

inline std::wstring get_module_folderpathw(HMODULE mod = nullptr)
{
    wchar_t buffer[MAX_PATH + 1];
    DWORD   actual_length = GetModuleFileNameW(mod, buffer, MAX_PATH);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        const DWORD  long_path_length = 0xFFFF; // should be always enough
        std::wstring long_filename(long_path_length, L'\0');
        actual_length = GetModuleFileNameW(mod, long_filename.data(), long_path_length);
        PathRemoveFileSpecW(long_filename.data());
        long_filename.resize(std::wcslen(long_filename.data()));
        long_filename.shrink_to_fit();
        return long_filename;
    }

    PathRemoveFileSpecW(buffer);
    return {buffer, static_cast<uint64_t>(lstrlenW(buffer))};
}

inline std::string get_module_folderpatha(HMODULE mod = nullptr)
{
    char  buffer[MAX_PATH + 1];
    DWORD actual_length = GetModuleFileNameA(mod, buffer, MAX_PATH);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        const DWORD long_path_length = 0xFFFF; // should be always enough
        std::string long_filename(long_path_length, L'\0');
        actual_length = GetModuleFileNameA(mod, long_filename.data(), long_path_length);
        PathRemoveFileSpecA(long_filename.data());
        long_filename.resize(strlen(long_filename.data()));
        long_filename.shrink_to_fit();
        return long_filename;
    }

    PathRemoveFileSpecA(buffer);
    return {buffer, static_cast<uint64_t>(lstrlenA(buffer))};
}
} // namespace Paths
