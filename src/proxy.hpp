#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCApplication.hpp>
#include "gd-file-picker.h"
#include "winepath.hpp"

using namespace geode::prelude;

std::wstring to_wstring(const char* str);
std::wstring to_wstring(const std::string& str);

inline HANDLE g_pickerProxyProc;
inline HANDLE g_watchdogMutex = NULL;
inline GDFilePicker* g_filePicker = nullptr;