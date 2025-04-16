#include "network.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>
#include "ui.h"

void Network::logEvent(const std::string& clientIP, const std::string& query, const std::string& response) {
    std::ofstream log_file("log.log", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de log." << std::endl;
        return;
    }

    time_t now = time(0);
    struct tm tstruct;
    char date_buf[11];
    char time_buf[9];
    localtime_s(&tstruct, &now);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

    std::string safe_query = query;
    std::string safe_response = response;
    std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
    std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

    log_file << date_buf << " " << time_buf << " " << clientIP << " "
             << safe_query << " " << safe_response << std::endl;
}

std::string Network::sendMessage(SOCKET client_socket, const std::string& clientIP, const std::string& message) {
    if (message.empty()) return "";

    if (send(client_socket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
        logEvent(clientIP, message, "ERROR|SEND_FAILED");
        throw std::runtime_error("Error enviando mensaje al servidor.");
    }

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        logEvent(clientIP, message, "ERROR|RECV_FAILED");
        return "ERROR|CONNECTION_LOST";
    }

    buffer[bytes_received] = '\0';
    std::string response(buffer, bytes_received);
    logEvent(clientIP, message, response);

    return response;
}

std::string Network::getLocalIP() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error al iniciar Winsock." << std::endl;
        return "UNKNOWN";
    }

    char hostname[NI_MAXHOST];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error al obtener hostname: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return "UNKNOWN";
    }

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, nullptr, &hints, &res) != 0) {
        std::cerr << "Error en getaddrinfo: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return "UNKNOWN";
    }

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    freeaddrinfo(res);
    WSACleanup();

    std::string ip = std::string(ip_str);
    if (ip.empty() || ip == "0.0.0.0") {
        return "UNKNOWN";
    }
    return ip;
}

SOCKET Network::connectToServer(const std::string& server_ip, int server_port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error al iniciar Winsock." << std::endl;
        throw std::runtime_error("Error al iniciar Winsock.");
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr);

    bool connected = false;
    auto start_time = std::chrono::steady_clock::now();

    std::cout << "Intentando conectar con el servidor..." << std::endl;

    while (!connected) {
        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == 0) {
            connected = true;
            break;
        }

        closesocket(client_socket);
        client_socket = socket(AF_INET, SOCK_STREAM, 0);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 15) {
            WSACleanup();
            throw std::runtime_error("No fue posible conectar con el servidor después de 15 segundos.");
        }

        std::cout << "." << std::flush;
    }

    std::cout << "\nConexión establecida con éxito." << std::endl;
    return client_socket;
}

void Network::cleanup(SOCKET client_socket) {
    closesocket(client_socket);
    WSACleanup();
}