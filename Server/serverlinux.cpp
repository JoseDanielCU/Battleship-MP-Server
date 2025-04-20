#include <iostream>
// Incluye la biblioteca para manejo de hilos, permitiendo el uso de std::thread.
#include <thread>
// Incluye la biblioteca para contenedores de vectores, como std::vector.
#include <vector>
// Incluye la biblioteca para sockets, permitiendo operaciones de red como socket().
#include <sys/socket.h>
// Incluye la biblioteca para direcciones de red, como sockaddr_in.
#include <netinet/in.h>
// Incluye la biblioteca para conversiones de direcciones IP, como inet_pton().
#include <arpa/inet.h>
// Incluye la biblioteca para manejo de descriptores de archivo, como close().
#include <unistd.h>
// Incluye la biblioteca para manejo de señales, como signal().
#include <signal.h>
// Incluye la biblioteca para manejo de cadenas de C, como strerror().
#include <cstring>
// Máximo número de clientes conectados simultáneamente.
#define MAX_CLIENTS 100
// Incluye la definición del estado del servidor.
#include "server_state.h"
// Incluye la definición del manejador de clientes.
#include "client_handler.h"
// Incluye la definición del emparejamiento.
#include "matchmaking.h"

// Variable global para controlar la ejecución del servidor.
volatile sig_atomic_t server_running = 1;

// Manejador de señales para cerrar el servidor limpiamente.
void signal_handler(int sig) {
    server_running = 0;
}

int main(int argc, char* argv[]) {
    // Verifica los argumentos de entrada.
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <IP> <PORT> </path/log.log>" << std::endl;
        return 1;
    }
    // Inicializa el estado del servidor.
    ServerState state;
    std::string ip_address = argv[1];
    int port = std::stoi(argv[2]);
    state.log_path = argv[3];
    std::cout << "Iniciando servidor Battleship..." << std::endl;
    // Configura manejadores de señales.
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    // Carga la base de datos de usuarios.
    load_users(state);
    // Crea el socket del servidor.
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error al crear socket: " << strerror(errno) << std::endl;
        return 1;
    }
    // Configura la opción SO_REUSEADDR para reutilizar el puerto.
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Error al configurar SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(server_socket);
        return 1;
    }
    // Configura la dirección del servidor.
    struct sockaddr_in address = { AF_INET, htons(port), {} };
    if (inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr) <= 0) {
        std::cerr << "Dirección IP inválida: " << ip_address << std::endl;
        close(server_socket);
        return 1;
    }
    // Vincula el socket a la dirección y puerto.
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
        std::cerr << "Error al bind: " << strerror(errno) << std::endl;
        close(server_socket);
        return 1;
    }
    // Escucha conexiones entrantes.
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        std::cerr << "Error al listen: " << strerror(errno) << std::endl;
        close(server_socket);
        return 1;
    }
    std::cout << "Servidor iniciado y escuchando en " << ip_address << ":" << port << std::endl;
    // Inicia el hilo de emparejamiento.
    std::thread matchmaking_thread(matchmaking, std::ref(state));
    matchmaking_thread.detach();
    std::cout << "Hilo de matchmaking iniciado." << std::endl;
    // Lista de hilos de clientes.
    std::vector<std::thread> client_threads;
    while (server_running) {
        // Acepta nuevas conexiones.
        int new_socket = accept(server_socket, nullptr, nullptr);
        if (new_socket == -1) {
            if (server_running) {
                std::cerr << "Error al aceptar conexión: " << strerror(errno) << std::endl;
            }
            continue;
        }
        std::cout << "Nuevo cliente conectado." << std::endl;
        // Crea un hilo para manejar al cliente.
        client_threads.emplace_back(handle_client, new_socket, std::ref(state));
        // Limpia hilos terminados.
        if (client_threads.size() > 10) {
            std::vector<std::thread> active_threads;
            for (auto& t : client_threads) {
                if (t.joinable()) {
                    t.join();
                } else {
                    active_threads.push_back(std::move(t));
                }
            }
            client_threads = std::move(active_threads);
        }
    }
    std::cout << "Cerrando servidor..." << std::endl;
    close(server_socket);
    return 0;
}