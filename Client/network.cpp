#include "network.h" // Incluye el encabezado con definiciones de red
#include <iostream> // Incluye la biblioteca para entrada/salida
#include <fstream> // Incluye la biblioteca para manejo de archivos
#include <ctime> // Incluye la biblioteca para manejo de tiempo
#include <thread> // Incluye la biblioteca para hilos
#include <chrono> // Incluye la biblioteca para mediciones de tiempo
#include "ui.h" // Incluye funciones de interfaz de usuario

// Registra un evento en el archivo de log
void Network::logEvent(const std::string& clientIP, const std::string& query, const std::string& response) {
    // Abre el archivo de log en modo append
    std::ofstream log_file("log.log", std::ios::app);
    // Verifica si el archivo se abrió correctamente
    if (!log_file.is_open()) {
        // Muestra un mensaje de error si falla
        std::cerr << "Error: No se pudo abrir el archivo de log." << std::endl;
        return;
    }

    // Obtiene la fecha y hora actual
    time_t now = time(0);
    struct tm tstruct;
    char date_buf[11];
    char time_buf[9];
    localtime_s(&tstruct, &now);
    // Formatea la fecha (YYYY-MM-DD)
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
    // Formatea la hora (HH:MM:SS)
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

    // Crea copias seguras de la consulta y respuesta
    std::string safe_query = query;
    std::string safe_response = response;
    // Reemplaza espacios por guiones bajos en la consulta
    std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
    // Reemplaza espacios por guiones bajos en la respuesta
    std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

    // Escribe el evento en el archivo de log
    log_file << date_buf << " " << time_buf << " " << clientIP << " "
             << safe_query << " " << safe_response << std::endl;
}

// Envía un mensaje al servidor y espera una respuesta
std::string Network::sendMessage(SOCKET client_socket, const std::string& clientIP, const std::string& message) {
    // Verifica si el mensaje está vacío
    if (message.empty()) return "";

    // Envía el mensaje al servidor a través del socket
    // Protocolo: el mensaje sigue el formato BSS/1.0, ej. "LOGIN|username|password"
    if (send(client_socket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
        // Registra un error si falla el envío
        logEvent(clientIP, message, "ERROR|SEND_FAILED");
        // Lanza una excepción con el error
        throw std::runtime_error("Error enviando mensaje al servidor.");
    }

    // Buffer para recibir la respuesta del servidor
    char buffer[1024] = {0};
    // Recibe la respuesta del servidor
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    // Verifica si la recepción falló o la conexión se cerró
    if (bytes_received <= 0) {
        // Registra un error de recepción
        logEvent(clientIP, message, "ERROR|RECV_FAILED");
        // Devuelve un mensaje de error estándar
        return "ERROR|CONNECTION_LOST";
    }

    // Asegura que el buffer termine en null
    buffer[bytes_received] = '\0';
    // Convierte el buffer a una cadena
    std::string response(buffer, bytes_received);
    // Registra el evento en el log
    logEvent(clientIP, message, response);

    // Devuelve la respuesta del servidor
    return response;
}

// Obtiene la IP local del cliente
std::string Network::getLocalIP() {
    // Estructura para inicializar Winsock
    WSADATA wsaData;
    // Inicializa Winsock versión 2.2
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // Muestra un mensaje de error si falla
        std::cerr << "Error al iniciar Winsock." << std::endl;
        return "UNKNOWN";
    }

    // Buffer para almacenar el nombre del host
    char hostname[NI_MAXHOST];
    // Obtiene el nombre del host local
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        // Muestra un mensaje de error si falla
        std::cerr << "Error al obtener hostname: " << WSAGetLastError() << std::endl;
        // Limpia Winsock
        WSACleanup();
        return "UNKNOWN";
    }

    // Estructuras para obtener información de la dirección
    struct addrinfo hints{}, *res = nullptr;
    // Configura los parámetros: familia IPv4, socket TCP
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Obtiene la información de la dirección del host
    if (getaddrinfo(hostname, nullptr, &hints, &res) != 0) {
        // Muestra un mensaje de error si falla
        std::cerr << "Error en getaddrinfo: " << WSAGetLastError() << std::endl;
        // Limpia Winsock
        WSACleanup();
        return "UNKNOWN";
    }

    // Buffer para la dirección IP
    char ip_str[INET_ADDRSTRLEN];
    // Convierte la dirección a formato legible
    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    // Libera los recursos de getaddrinfo
    freeaddrinfo(res);
    // Limpia Winsock
    WSACleanup();

    // Convierte la IP a una cadena
    std::string ip = std::string(ip_str);
    // Verifica si la IP es válida
    if (ip.empty() || ip == "0.0.0.0") {
        return "UNKNOWN";
    }
    // Devuelve la IP local
    return ip;
}

// Conecta al servidor y devuelve el socket
SOCKET Network::connectToServer(const std::string& server_ip, int server_port) {
    // Estructura para inicializar Winsock
    WSADATA wsaData;
    // Inicializa Winsock versión 2.2
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // Muestra un mensaje de error si falla
        std::cerr << "Error al iniciar Winsock." << std::endl;
        // Lanza una excepción
        throw std::runtime_error("Error al iniciar Winsock.");
    }

    // Crea un socket TCP
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Estructura para la dirección del servidor
    struct sockaddr_in server_address{};
    // Configura la familia de direcciones (IPv4)
    server_address.sin_family = AF_INET;
    // Convierte el puerto a formato de red
    server_address.sin_port = htons(server_port);
    // Convierte la IP a formato binario
    inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr);

    // Bandera para indicar si la conexión fue exitosa
    bool connected = false;
    // Registra el tiempo de inicio para el temporizador
    auto start_time = std::chrono::steady_clock::now();

    // Muestra un mensaje de intento de conexión
    std::cout << "Intentando conectar con el servidor..." << std::endl;

    // Bucle para intentar conectar
    while (!connected) {
        // Intenta conectar al servidor
        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == 0) {
            // Si la conexión es exitosa, sale del bucle
            connected = true;
            break;
        }

        // Cierra el socket actual
        closesocket(client_socket);
        // Crea un nuevo socket
        client_socket = socket(AF_INET, SOCK_STREAM, 0);

        // Espera 1 segundo antes de reintentar
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // Calcula el tiempo transcurrido
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        // Si han pasado 15 segundos, lanza una excepción
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 15) {
            // Limpia Winsock
            WSACleanup();
            throw std::runtime_error("No fue posible conectar con el servidor después de 15 segundos.");
        }

        // Muestra un punto para indicar que sigue intentando
        std::cout << "." << std::flush;
    }

    // Muestra un mensaje de conexión exitosa
    std::cout << "\nConexión establecida con éxito." << std::endl;
    // Devuelve el socket conectado
    return client_socket;
}

// Limpia el socket y los recursos de Winsock
void Network::cleanup(SOCKET client_socket) {
    // Cierra el socket
    closesocket(client_socket);
    // Limpia Winsock
    WSACleanup();
}