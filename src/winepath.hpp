#pragma once
#include <windows.h>
#include <string>


inline LPSTR (*CDECL wine_get_unix_file_name_ptr)(LPCWSTR) = NULL;
inline LPWSTR (*CDECL wine_get_dos_file_name_ptr)(LPCSTR) = NULL;

std::string unixToDosPath(const std::string& unixPath);
std::string dosToUnixPath(const std::string& dosPath);