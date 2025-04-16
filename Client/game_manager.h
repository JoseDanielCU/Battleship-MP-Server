#ifndef GAME_MANAGER_H // Directiva para evitar inclusión múltiple
#define GAME_MANAGER_H
#include "game_common.h" // Incluye definiciones comunes del juego
#include "network.h" // Incluye funciones de red
#include "ui.h" // Incluye funciones de interfaz de usuario
#include <string> // Incluye la biblioteca para cadenas
#include <winsock2.h> // Incluye la biblioteca de Winsock para sockets

// Clase que gestiona la lógica del juego del cliente
class GameManager {
public:
    // Inicia una partida de Battleship
    static void startGame(SOCKET client_socket, const std::string& clientIP, const std::string& username);
    // Muestra el menú de login y maneja la autenticación
    static bool loginMenu(SOCKET client_socket, std::string& username, const std::string& clientIP);
    // Busca una partida en el servidor
    static void searchGame(SOCKET client_socket, const std::string& clientIP, const std::string& username);
    // Muestra el menú para usuarios autenticados
    static void menuLoggedIn(SOCKET client_socket, const std::string& clientIP, std::string& username);
};

#endif // Cierra la directiva de inclusión