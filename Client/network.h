#ifndef NETWORK_H
#define NETWORK_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

class Network {
public:
    static std::string sendMessage(SOCKET client_socket, const std::string& clientIP, const std::string& message);
    static std::string getLocalIP();
    static SOCKET connectToServer(const std::string& server_ip, int server_port);
    static void cleanup(SOCKET client_socket);
    static void logEvent(const std::string& clientIP, const std::string& query, const std::string& response);
};

#endif