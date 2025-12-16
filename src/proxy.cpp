#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCApplication.hpp>

using namespace geode::prelude;

HANDLE g_pickerProxyProc;
HANDLE g_watchdogMutex = NULL;

class $modify(PickerApplication, CCApplication) {
    void platformShutdown() {
        if (g_pickerProxyProc != NULL) {
            if (WaitForSingleObject(g_pickerProxyProc, 1000) != WAIT_OBJECT_0) {
                TerminateProcess(g_pickerProxyProc, 0);
                WaitForSingleObject(g_pickerProxyProc, INFINITE);
            }
            CloseHandle(g_pickerProxyProc);
            g_pickerProxyProc = NULL;
        }
        CCApplication::platformShutdown();
    }
};

class $modify(PickerLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }
        runAction(CCCallFunc::create(this, callfunc_selector(PickerLayer::launchLinuxServer)));
        return true;
    }

    void launchLinuxServer() {
        if (g_pickerProxyProc != NULL) {
            return;
        } 

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        std::string path = CCFileUtils::get()->fullPathForFilename("starter-proxy.exe.so"_spr, true);
        std::string picker_server_path = CCFileUtils::get()->fullPathForFilename("picker-linux-side"_spr, true);

        g_watchdogMutex = CreateMutexA(NULL, TRUE, "PickerWatchdogMutex");
        if (g_watchdogMutex == NULL) {
            log::error("Failed to create watchdog mutex: {}", GetLastError());
            return;
        }

        CopyFileA(picker_server_path.c_str(), "Z:\\tmp\\picker-linux-side", FALSE);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		if (!CreateProcess(path.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
			log::error("Failed to launch Linux input program: {}", GetLastError());
            CloseHandle(g_watchdogMutex);
            g_watchdogMutex = NULL;
			return;
		}

        DWORD exitCode = 0;
        if (WaitForSingleObject(pi.hProcess, 200) == WAIT_OBJECT_0) {
            GetExitCodeProcess(pi.hProcess, &exitCode);
            log::info("Process exited immediately with code {}", exitCode);
            CloseHandle(g_watchdogMutex);
            g_watchdogMutex = NULL;
        } else {
            g_pickerProxyProc = pi.hProcess;
            log::info("Process is running.");
        }
    }

};
