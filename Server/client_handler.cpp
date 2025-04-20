#include "client_handler.h"
// Incluye la biblioteca estándar de C++ para entrada/salida, como std::cout y std::cin.
#include <iostream>
// Incluye la biblioteca para manipulación de flujos de cadenas, como std::stringstream.
#include <sstream>
// Incluye la biblioteca para sockets, permitiendo operaciones de red como recv().
#include <sys/socket.h>
// Incluye la biblioteca para manejo de descriptores de archivo, como close().
#include <unistd.h>
// Incluye la biblioteca para control de descriptores de archivo, como fcntl().
#include <fcntl.h>
// Incluye la biblioteca para manejo de cadenas de C, como strlen().
#include <cstring>
// Incluye la biblioteca para manejo de errores, como strerror().
#include <errno.h>
// Incluye la definición de los protocolos.
#include "protocols.h"

// Maneja la comunicación con un cliente conectado.
void handle_client(int client_socket, ServerState& state) {
    char buffer[4096] = {0};
    std::string logged_user;
    // Configura el socket como bloqueante.
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);

    while (true) {
        // Recibe datos del cliente.
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            // Cliente desconectado o error.
            if (!logged_user.empty()) {
                process_protocols("LOGOUT", logged_user, "", client_socket, logged_user, state);
                log_event(state, "Usuario desconectado y deslogueado: " + logged_user + " (recv falló con " +
                          std::to_string(bytes_read) + ", error: " + std::string(strerror(errno)) + ")");
                std::cout << "Cliente desconectado y deslogueado: " << logged_user << std::endl;
            } else {
                log_event(state, "Cliente anónimo desconectado (recv falló con " + std::to_string(bytes_read) +
                          ", error: " + std::string(strerror(errno)) + ")");
                std::cout << "Cliente anónimo desconectado." << std::endl;
            }
            close(client_socket);
            return;
        }
        buffer[bytes_read] = '\0';
        std::string message(buffer, bytes_read);
        std::istringstream msg_stream(message);
        std::string command, param1, param2;
        // Parsea el mensaje en comando y parámetros.
        std::getline(msg_stream, command, '|');
        std::getline(msg_stream, param1, '|');
        std::getline(msg_stream, param2);
        // Configura el socket como no bloqueante después de LOGIN.
        if (command == "LOGIN" && !logged_user.empty()) {
            int flags = fcntl(client_socket, F_GETFL, 0);
            fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
        }
        // Procesa el comando recibido.
        process_protocols(command, param1, param2, client_socket, logged_user, state);
        if (command == "LOGOUT") {
            break;
        }
    }
}