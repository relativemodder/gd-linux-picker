
#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include "json.hpp"

#pragma comment(lib, "ws2_32.lib")

struct FileFilter {
    std::string name;
    std::vector<std::string> patterns;
};

struct OpenFileParams {
    bool multiple;
    std::vector<FileFilter> filters;
    std::optional<std::string> directory;
};

struct SaveFileParams {
    std::optional<std::string> filename;
};

struct OpenLocationParams {
    std::string path;
};

class GDFilePicker {
private:
    SOCKET m_socket;
    uint32_t m_nextId;
    bool m_connected;


    bool connect();
    std::string sendRequest(const std::string& method, const std::string& params);

public:
    GDFilePicker();
    ~GDFilePicker();

    std::vector<std::string> openFile(const OpenFileParams& params = {});
    std::vector<std::string> saveFile(const SaveFileParams& params = {});
    std::vector<std::string> openFolder();
    bool openLocation(const OpenLocationParams& params);

    bool isConnected() const { return m_connected; }
    void disconnect();
};
