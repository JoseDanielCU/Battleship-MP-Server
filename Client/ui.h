#ifndef UI_H // Directiva para evitar inclusión múltiple
#define UI_H
#include "game_common.h" // Incluye definiciones comunes del juego
#include <string> // Incluye la biblioteca para cadenas
#include <vector> // Incluye la biblioteca para vectores
#include <windows.h> // Incluye la biblioteca para funciones de Windows

// Clase que maneja la interfaz de usuario
class UI {
public:
    // Cambia el color del texto en la consola
    static void setConsoleColor(int color);
    // Restaura el color por defecto de la consola
    static void resetConsoleColor();
    // Dibuja un menú con un título y opciones
    static void drawMenu(const std::string& title, const std::vector<std::string>& options);
    // Muestra un mensaje de error
    static void showError(const std::string& message);
    // Muestra un mensaje de éxito
    static void showSuccess(const std::string& message);
    // Muestra un tablero de juego
    static void displayBoard(Board& board, bool hide_ships = false, bool is_opponent = false);
    // Configura el tablero del jugador
    static void setupBoard(Board& board);
    // Muestra una animación de espera
    static void waitingAnimation(bool& esperando);
};

#endif // Cierra la directiva de inclusión