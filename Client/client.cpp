#include "network.h" // Incluye el encabezado para funciones de red
#include "game_manager.h" // Incluye el encabezado para la lógica del juego
#include <iostream> // Incluye la biblioteca para entrada/salida en consola
#include <csignal> // Incluye la biblioteca para manejo de señales del sistema

// Declara una variable global para almacenar el socket del cliente
SOCKET global_client_socket = INVALID_SOCKET;
// Declara una variable global para almacenar el nombre de usuario
std::string global_username;
// Declara una variable global para almacenar la IP del cliente
std::string global_clientIP;

// Define un manejador de eventos de consola para señales como Ctrl+C
BOOL WINAPI ConsoleHandler(DWORD signal) {
    // Verifica si la señal es Ctrl+C o cierre de consola
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        // Comprueba si el socket es válido y el nombre de usuario no está vacío
        if (global_client_socket != INVALID_SOCKET && !global_username.empty()) {
            try {
                // Envía un mensaje de LOGOUT al servidor con el nombre de usuario
                // Protocolo: "LOGOUT|username" indica que el cliente se desconecta
                Network::sendMessage(global_client_socket, global_clientIP, "LOGOUT|" + global_username);
            } catch (const std::runtime_error&) {
                // Ignora cualquier error al enviar el mensaje LOGOUT
            }
            // Limpia el socket y recursos de red
            Network::cleanup(global_client_socket);
        }
        // Termina el programa
        exit(0);
    }
    // Indica que la señal fue manejada
    return TRUE;
}

// Función principal del programa
int main() {
    // Configura la codificación de la consola a UTF-8 para soportar caracteres especiales
    SetConsoleOutputCP(CP_UTF8);
    // Desactiva la sincronización de std::cin para mejorar el rendimiento
    std::cin.tie(nullptr);
    // Define la IP del servidor (hardcoded)
    const std::string server_ip = "3.210.99.80";
    // Define el puerto del servidor (hardcoded)0...
    const int server_port = 8080;

    // Registra el manejador de consola para capturar señales
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        // Muestra un mensaje de error si falla la configuración
        std::cerr << "Error al configurar el manejador de consola." << std::endl;
        return 1; // Termina con código de error
    }

    try {
        // Obtiene la IP local del cliente
        global_clientIP = Network::getLocalIP();
        // Establece una conexión con el servidor y almacena el socket
        global_client_socket = Network::connectToServer(server_ip, server_port);

        // Bucle principal del cliente
        while (true) {
            // Limpia el nombre de usuario para un nuevo intento de login
            global_username.clear();
            // Muestra el menú de login y maneja el proceso de autenticación
            // Si loginMenu devuelve false, el usuario eligió salir
            if (!GameManager::loginMenu(global_client_socket, global_username, global_clientIP)) break;
            // Muestra el menú para usuarios autenticados
            GameManager::menuLoggedIn(global_client_socket, global_clientIP, global_username);
        }

        // Si hay un nombre de usuario, envía un mensaje de LOGOUT al cerrar
        if (!global_username.empty()) {
            // Protocolo: "LOGOUT|username" notifica al servidor la desconexión
            Network::sendMessage(global_client_socket, global_clientIP, "LOGOUT|" + global_username);
        }
        // Limpia el socket y recursos de red
        Network::cleanup(global_client_socket);
    } catch (const std::runtime_error& e) {
        // Captura cualquier error durante la ejecución
        std::cerr << "Error: " << e.what() << std::endl;
        // Si el socket es válido, lo limpia
        if (global_client_socket != INVALID_SOCKET) {
            Network::cleanup(global_client_socket);
        }
        return 1; // Termina con código de error
    }

    return 0; // Termina exitosamente
}