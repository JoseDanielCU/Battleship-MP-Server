#include "ui.h" // Incluye el encabezado de la clase UI
#include <iostream> // Incluye la biblioteca para entrada/salida
#include <iomanip> // Incluye la biblioteca para formato de salida
#include <cstdlib> // Incluye la biblioteca para funciones estándar
#include <thread> // Incluye la biblioteca para hilos
#include <chrono> // Incluye la biblioteca para mediciones de tiempo
#include "game_common.h" // Incluye definiciones comunes del juego

// Cambia el color del texto en la consola
void UI::setConsoleColor(int color) {
    // Obtiene el manejador de la consola
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // Establece el color del texto
    SetConsoleTextAttribute(hConsole, color);
}

// Restaura el color por defecto de la consola
void UI::resetConsoleColor() {
    // Establece el color blanco (7)
    setConsoleColor(7);
}

// Dibuja un menú con un título y opciones
void UI::drawMenu(const std::string& title, const std::vector<std::string>& options) {
    // Limpia la consola
    system("cls");
    // Define el ancho del menú
    const int width = 40;
    // Crea una línea de borde
    std::string border = std::string(width, '=');

    // Muestra el borde superior
    std::cout << border << std::endl;
    // Muestra el título alineado a la izquierda
    std::cout << std::setw(width) << std::left << "  " + title << std::endl;
    // Muestra el borde inferior del título
    std::cout << border << std::endl;

    // Muestra las opciones numeradas
    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << options[i] << std::endl;
    }

    // Muestra el borde inferior
    std::cout << border << std::endl;
    // Solicita la selección del usuario
    std::cout << "Seleccione una opción: ";
}

// Muestra un mensaje de error
void UI::showError(const std::string& message) {
    // Limpia la consola
    system("cls");
    // Muestra el encabezado de error
    std::cout << "====================\n";
    std::cout << "  ERROR\n";
    std::cout << "====================\n";
    // Muestra el mensaje
    std::cout << message << "\n";
    // Solicita presionar Enter
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

// Muestra un mensaje de éxito
void UI::showSuccess(const std::string& message) {
    // Limpia la consola
    system("cls");
    // Muestra el encabezado de éxito
    std::cout << "====================\n";
    std::cout << "  ÉXITO\n";
    std::cout << "====================\n";
    // Muestra el mensaje
    std::cout << message << "\n";
    // Solicita presionar Enter
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

// Muestra un tablero de juego
void UI::displayBoard(Board& board, bool hide_ships, bool is_opponent) {
    // Crea un resumen de los barcos
    std::vector<std::string> ship_summary;
    for (const auto& ship : board.ship_instances) {
        // Crea una línea con el nombre y estado del barco
        std::string line = ship.name + " (" + std::to_string(ship.positions.size()) + "): ";
        line += ship.sunk ? "Hundido" : "A flote";
        ship_summary.push_back(line);
    }
    // Define el encabezado según si es el tablero del oponente
    std::string header = is_opponent ? "Barcos del oponente:" : "Tus barcos:";
    // Inserta el encabezado al inicio
    ship_summary.insert(ship_summary.begin(), header);

    // Muestra la fila de índices de columnas
    std::cout << "\n  ";
    for (int i = 0; i < BOARD_SIZE; i++) std::cout << " " << i << " ";
    std::cout << "   | ";
    // Muestra el encabezado del resumen de barcos
    if (!ship_summary.empty()) std::cout << ship_summary[0];
    std::cout << "\n +";
    // Muestra el borde superior del tablero
    for (int i = 0; i < BOARD_SIZE; i++) std::cout << "---";
    std::cout << "+  |";

    // Contador para las líneas del resumen de barcos
    size_t ship_line = 1;
    // Itera sobre las filas del tablero
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Muestra el índice de la fila
        std::cout << "\n" << i << "|";
        // Itera sobre las columnas
        for (int j = 0; j < BOARD_SIZE; j++) {
            // Obtiene el símbolo de la casilla
            char c = board.grid[i][j];
            // Si se deben ocultar los barcos, muestra agua en lugar de barcos
            if (hide_ships && c != HIT && c != MISS && c != WATER) c = WATER;
            // Muestra un impacto
            if (c == HIT) {
                setConsoleColor(12); // Rojo
                std::cout << " X ";
                resetConsoleColor();
            } else if (c == MISS) {
                // Muestra un fallo
                setConsoleColor(9); // Azul
                std::cout << " O ";
                resetConsoleColor();
            } else if (c == WATER) {
                // Muestra agua
                setConsoleColor(11); // Cian
                std::cout << " ~ ";
                resetConsoleColor();
            } else {
                // Muestra un barco
                setConsoleColor(10); // Verde
                std::cout << " " << c << " ";
                resetConsoleColor();
            }
        }
        std::cout << "|  |";
        // Muestra la siguiente línea del resumen de barcos
        if (ship_line < ship_summary.size()) {
            std::cout << ship_summary[ship_line];
            ship_line++;
        }
    }
    // Muestra el borde inferior del tablero
    std::cout << "\n +";
    for (int i = 0; i < BOARD_SIZE; i++) std::cout << "---";
    std::cout << "+  |";
    // Muestra las líneas restantes del resumen de barcos
    for (; ship_line < ship_summary.size(); ship_line++) {
        std::cout << "\n   " << std::string(BOARD_SIZE * 3, ' ') << "   |" << ship_summary[ship_line];
    }
    std::cout << "\n";
}

// Configura el tablero del jugador
void UI::setupBoard(Board& board) {
    // Limpia la consola
    system("cls");
    // Muestra el encabezado de configuración
    std::cout << "==================================\n";
    std::cout << "  CONFIGURACIÓN DEL TABLERO\n";
    std::cout << "==================================\n";
    // Solicita el método de colocación
    std::cout << "Desea colocar los barcos manualmente (M) o aleatoriamente (A)? ";
    std::string choice;
    // Lee la elección del usuario
    std::getline(std::cin, choice);
    // Convierte la elección a mayúscula, por defecto 'A'
    choice = toupper(choice.empty() ? 'A' : choice[0]);

    // Si el usuario elige colocación manual
    if (choice == "M") {
        // Itera sobre cada tipo de barco
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            // Bucle hasta que el barco se coloque correctamente
            while (!placed) {
                // Limpia la consola
                system("cls");
                // Muestra el tablero actual
                displayBoard(board);
                // Muestra el barco que se está colocando
                std::cout << "Colocando " << ship.name << " (" << ship.size << " casillas)." << std::endl;

                // Solicita la fila inicial
                std::cout << "Ingrese fila inicial (0-9): ";
                std::string input;
                std::getline(std::cin, input);

                try {
                    // Convierte la entrada a número
                    x = std::stoi(input);
                    // Verifica si la fila es válida
                    if (x < 0 || x >= BOARD_SIZE) {
                        std::cout << "Fila inválida. Debe estar entre 0 y 9. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                        continue;
                    }
                } catch (const std::exception&) {
                    // Maneja entradas no numéricas
                    std::cout << "Entrada no numérica. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }

                // Solicita la columna inicial
                std::cout << "Ingrese columna inicial (0-9): ";
                std::getline(std::cin, input);

                try {
                    // Convierte la entrada a número
                    y = std::stoi(input);
                    // Verifica si la columna es válida
                    if (y < 0 || y >= BOARD_SIZE) {
                        std::cout << "Columna inválida. Debe estar entre 0 y 9. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                        continue;
                    }
                } catch (const std::exception&) {
                    // Maneja entradas no numéricas
                    std::cout << "Entrada no numérica. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }

                // Solicita la dirección
                std::cout << "Ingrese dirección (H/V): ";
                std::getline(std::cin, input);
                if (input.empty()) {
                    // Maneja entradas vacías
                    std::cout << "Entrada vacía. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }
                // Convierte la dirección a mayúscula
                direction = toupper(input[0]);

                // Verifica si la dirección es válida
                if (direction == 'H' || direction == 'V') {
                    // Verifica si el barco puede colocarse
                    if (board.canPlaceShip(x, y, ship.size, direction)) {
                        // Coloca el barco en el tablero
                        board.placeShip(x, y, ship.size, direction, ship.symbol);
                        // Rastrea las posiciones del barco
                        board.trackShip(ship, x, y, direction);
                        // Marca el barco como colocado
                        placed = true;
                    } else {
                        // Muestra un mensaje de error
                        std::cout << "No se puede colocar el barco ahí. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                    }
                } else {
                    // Muestra un mensaje de error
                    std::cout << "Dirección inválida. Use 'H' o 'V'. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                }
            }
        }
    } else {
        // Colocación aleatoria
        // Inicializa la semilla para números aleatorios
        srand(time(0));
        // Itera sobre cada tipo de barco
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            // Bucle hasta que el barco se coloque
            while (!placed) {
                // Genera coordenadas aleatorias
                x = rand() % BOARD_SIZE;
                y = rand() % BOARD_SIZE;
                // Genera una dirección aleatoria
                direction = (rand() % 2 == 0) ? 'H' : 'V';

                // Verifica si el barco puede colocarse
                if (board.canPlaceShip(x, y, ship.size, direction)) {
                    // Coloca el barco en el tablero
                    board.placeShip(x, y, ship.size, direction, ship.symbol);
                    // Rastrea las posiciones del barco
                    board.trackShip(ship, x, y, direction);
                    // Marca el barco como colocado
                    placed = true;
                }
            }
        }
    }

    // Limpia la consola
    system("cls");
    // Muestra el tablero configurado
    std::cout << "Tu tablero:\n";
    displayBoard(board);
    // Solicita presionar Enter
    std::cout << "Tablero configurado. Presione Enter para continuar...";
    std::cin.get();
}

// Muestra una animación de espera
void UI::waitingAnimation(bool& esperando) {
    // Arreglo de frames para la animación
    const char* anim[] = {"[ / ]", "[ - ]", "[ \\ ]", "[ | ]"};
    int i = 0;
    // Bucle mientras se está esperando
    while (esperando) {
        // Muestra el frame actual
        std::cout << "\rBuscando oponente... " << anim[i % 4] << std::flush;
        // Espera 200 milisegundos
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // Avanza al siguiente frame
        i++;
    }
    // Limpia la línea de la animación
    std::cout << "\r                                    \r" << std::flush;
}