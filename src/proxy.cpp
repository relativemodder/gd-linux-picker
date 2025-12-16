
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCApplication.hpp>
#include "gd-file-picker.h"

using namespace geode::prelude;


HANDLE g_pickerProxyProc;
HANDLE g_watchdogMutex = NULL;
GDFilePicker* g_filePicker = nullptr;


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
        
        if (g_filePicker != nullptr) {
            delete g_filePicker;
            g_filePicker = nullptr;
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

        auto myButton = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("folderIcon_001.png"),
			this,
			menu_selector(PickerLayer::onMyButton)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);
		myButton->setID("test-picker-button"_spr);
		menu->updateLayout();

        return true;
    }

    void onMyButton(CCObject*) {
		testOpenFile();
	}

    void launchLinuxServer() {
        if (g_pickerProxyProc != NULL) {
            return;
        } 

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        std::string path = CCFileUtils::get()->fullPathForFilename("starterproxy.exe.so"_spr, true);
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
			log::error("Failed to launch Linux picker program: {}", GetLastError());
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
            
            try {
                g_filePicker = new GDFilePicker();
                log::info("File picker initialized");
            } catch (const std::exception& e) {
                log::error("Failed to initialize file picker: {}", e.what());
            }
        }
    }


    void testOpenFile() {
        if (!g_filePicker) return;
        
        try {
            OpenFileParams params;
            params.multiple = true;
            auto files = g_filePicker->openFile(params);
            log::info("Selected {} files", files.size());

            for (auto file : files) {
                log::debug("File {};", file);
            }

        } catch (const std::exception& e) {
            log::error("Failed to open file: {}", e.what());
        }
    }

    void testSaveFile() {
        if (!g_filePicker) return;
        
        try {
            SaveFileParams params;
            params.filename = "level.txt";
            auto files = g_filePicker->saveFile(params);
            log::info("Save path: {}", files.empty() ? "cancelled" : files[0]);
        } catch (const std::exception& e) {
            log::error("Failed to save file: {}", e.what());
        }
    }

    void testOpenFolder() {
        if (!g_filePicker) return;
        
        try {
            auto folders = g_filePicker->openFolder();
            log::info("Selected folder: {}", folders.empty() ? "cancelled" : folders[0]);
        } catch (const std::exception& e) {
            log::error("Failed to open folder: {}", e.what());
        }
    }
};
