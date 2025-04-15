#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

std::string get_local_ip() {
    char hostname[NI_MAXHOST];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error al obtener hostname: " << WSAGetLastError() << std::endl;
        return "UNKNOWN";
    }

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, nullptr, &hints, &res) != 0) {
        std::cerr << "Error en getaddrinfo: " << WSAGetLastError() << std::endl;
        return "UNKNOWN";
    }

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    freeaddrinfo(res);

    return std::string(ip_str);
}

std::string send_message(SOCKET client_socket, const std::string& clientIP, const std::string& message) {
    if (message.empty()) return "";

    if (send(client_socket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
        std::ofstream log_file("log.log", std::ios::app);
        if (log_file.is_open()) {
            time_t now = time(0);
            struct tm tstruct;
            char date_buf[11];
            char time_buf[9];
            localtime_s(&tstruct, &now);
            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

            std::string safe_query = message;
            std::string safe_response = "ERROR|SEND_FAILED";
            std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
            std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

            log_file << date_buf << " " << time_buf << " " << clientIP << " "
                     << safe_query << " " << safe_response << std::endl;
        }
        throw std::runtime_error("Error enviando mensaje al servidor.");
    }

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::ofstream log_file("log.log", std::ios::app);
        if (log_file.is_open()) {
            time_t now = time(0);
            struct tm tstruct;
            char date_buf[11];
            char time_buf[9];
            localtime_s(&tstruct, &now);
            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

            std::string safe_query = message;
            std::string safe_response = "ERROR|RECV_FAILED";
            std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
            std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

            log_file << date_buf << " " << time_buf << " " << clientIP << " "
                     << safe_query << " " << safe_response << std::endl;
        }
        return "ERROR|CONNECTION_LOST";
    }

    buffer[bytes_received] = '\0';
    std::string response(buffer, bytes_received);
    std::ofstream log_file("log.log", std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(0);
        struct tm tstruct;
        char date_buf[11];
        char time_buf[9];
        localtime_s(&tstruct, &now);
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

        std::string safe_query = message;
        std::string safe_response = response;
        std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
        std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

        log_file << date_buf << " " << time_buf << " " << clientIP << " "
                 << safe_query << " " << safe_response << std::endl;
    }

    return response;
}