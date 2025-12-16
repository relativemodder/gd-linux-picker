
#include "gd-file-picker.h"

using nlohmann::json;

GDFilePicker::GDFilePicker() 
    : m_socket(INVALID_SOCKET), m_nextId(1), m_connected(false) {
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("Failed to initialize Winsock");
    }
}

GDFilePicker::~GDFilePicker() {
    disconnect();
    WSACleanup();
}

bool GDFilePicker::connect() {
    if (m_connected) return true;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9666);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (::connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_connected = true;
    return true;
}


std::string GDFilePicker::sendRequest(const std::string& method, const std::string& params) {
    if (!connect()) {
        return "{\"error\":\"Failed to connect to server\"}";
    }

    json requestJson;
    requestJson["id"] = m_nextId++;
    requestJson["method"] = method;
    
    try {
        requestJson["params"] = json::parse(params);
    } catch (...) {
        requestJson["params"] = json::object();
    }

    std::string request = requestJson.dump() + "\n";
    
    if (send(m_socket, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        m_connected = false;
        return "{\"error\":\"Failed to send request\"}";
    }

    char buffer[4096];
    int bytesReceived = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived <= 0) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        m_connected = false;
        return "{\"error\":\"Failed to receive response\"}";
    }

    buffer[bytesReceived] = '\0';
    return std::string(buffer);
}


std::vector<std::string> GDFilePicker::openFile(const OpenFileParams& params) {
    json paramsJson;
    paramsJson["multiple"] = params.multiple;
    paramsJson["filters"] = json::array();
    
    if (params.directory.has_value()) {
        paramsJson["directory"] = params.directory.value();
    }

    std::string response = sendRequest("open_file", paramsJson.dump());
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("error") && !responseJson["error"].is_null()) {
            throw std::runtime_error("Server error in openFile: " + responseJson["error"].get<std::string>());
        }
        
        if (responseJson.contains("result") && responseJson["result"].contains("paths")) {
            return responseJson["result"]["paths"].get<std::vector<std::string>>();
        }
        
        return std::vector<std::string>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse openFile response: " + std::string(e.what()));
    }
}


std::vector<std::string> GDFilePicker::saveFile(const SaveFileParams& params) {
    json paramsJson = json::object();
    
    if (params.filename.has_value()) {
        paramsJson["filename"] = params.filename.value();
    }

    std::string response = sendRequest("save_file", paramsJson.dump());
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("error") && !responseJson["error"].is_null()) {
            throw std::runtime_error("Server error in saveFile: " + responseJson["error"].get<std::string>());
        }
        
        if (responseJson.contains("result") && responseJson["result"].contains("paths")) {
            return responseJson["result"]["paths"].get<std::vector<std::string>>();
        }
        
        return std::vector<std::string>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse saveFile response: " + std::string(e.what()));
    }
}


std::vector<std::string> GDFilePicker::openFolder() {
    std::string response = sendRequest("open_folder", "{}");
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("error") && !responseJson["error"].is_null()) {
            throw std::runtime_error("Server error in openFolder: " + responseJson["error"].get<std::string>());
        }
        
        if (responseJson.contains("result") && responseJson["result"].contains("paths")) {
            return responseJson["result"]["paths"].get<std::vector<std::string>>();
        }
        
        return std::vector<std::string>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse openFolder response: " + std::string(e.what()));
    }
}


bool GDFilePicker::openLocation(const OpenLocationParams& params) {
    json paramsJson;
    paramsJson["path"] = params.path;
    
    std::string response = sendRequest("open_location", paramsJson.dump());
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("error") && !responseJson["error"].is_null()) {
            OutputDebugStringA(("Error opening location: " + responseJson["error"].get<std::string>()).c_str());
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        OutputDebugStringA(("Failed to parse openLocation response: " + std::string(e.what())).c_str());
        return false;
    }
}

void GDFilePicker::disconnect() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_connected = false;
}
