#include "winepath.hpp"

std::string unixToDosPath(const std::string& unixPath) {
    wine_get_dos_file_name_ptr = reinterpret_cast<LPWSTR (*)(LPCSTR)>(
            GetProcAddress(GetModuleHandleA("KERNEL32"),
                           "wine_get_dos_file_name"));

    if (!wine_get_dos_file_name_ptr) {
        throw std::runtime_error("wine_get_dos_file_name not found");
    }

    LPWSTR dosPath = wine_get_dos_file_name_ptr(unixPath.c_str());
    if (!dosPath) {
        throw std::runtime_error("Failed to convert Unix path to DOS path");
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, dosPath, -1, NULL, 0, NULL, NULL);
    std::string dosPathStr(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, dosPath, -1, &dosPathStr[0], size_needed, NULL, NULL);
    return dosPathStr;
}

std::string dosToUnixPath(const std::string& dosPath) {
    wine_get_unix_file_name_ptr = reinterpret_cast<LPSTR (*)(LPCWSTR)>(
            GetProcAddress(GetModuleHandleA("KERNEL32"),
                           "wine_get_unix_file_name"));

    if (!wine_get_unix_file_name_ptr) {
        throw std::runtime_error("wine_get_unix_file_name not found");
    }

    std::wstring wideDosPath(dosPath.begin(), dosPath.end());
    LPSTR unixPath = wine_get_unix_file_name_ptr(wideDosPath.c_str());
    if (!unixPath) {
        throw std::runtime_error("Failed to convert DOS path to Unix path");
    }

    return unixPath;
}