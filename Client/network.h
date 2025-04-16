#ifndef NETWORK_H // Directiva para evitar inclusión múltiple
#define NETWORK_H
#include <winsock2.h> // Incluye la biblioteca de Winsock
#include <ws2tcpip.h> // Incluye extensiones de Winsock para TCP/IP
#include <string> // Incluye la biblioteca para cadenas

// Clase que encapsula funciones de red
class Network {
public:
    // Envía un mensaje al servidor y devuelve la respuesta
    static std::string sendMessage(SOCKET client_socket, const std::string& clientIP, const std::string& message);
    // Obtiene la IP local del cliente
    static std::string getLocalIP();
    // Conecta al servidor y devuelve el socket
    static SOCKET connectToServer(const std::string& server_ip, int server_port);
    // Limpia el socket y los recursos de Winsock
    static void cleanup(SOCKET client_socket);
    // Registra un evento en el archivo de log
    static void logEvent(const std::string& clientIP, const std::string& query, const std::string& response);
};

#endif // Cierra la directiva de inclusión