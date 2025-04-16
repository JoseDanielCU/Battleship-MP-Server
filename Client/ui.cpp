#include "ui.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "game_common.h"

void UI::setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void UI::resetConsoleColor() {
    setConsoleColor(7); // Blanco (color por defecto)
}

void UI::drawMenu(const std::string& title, const std::vector<std::string>& options) {
    system("cls");
    const int width = 40;
    std::string border = std::string(width, '=');

    std::cout << border << std::endl;
    std::cout << std::setw(width) << std::left << "  " + title << std::endl;
    std::cout << border << std::endl;

    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << options[i] << std::endl;
    }

    std::cout << border << std::endl;
    std::cout << "Seleccione una opción: ";
}

void UI::showError(const std::string& message) {
    system("cls");
    std::cout << "====================\n";
    std::cout << "  ERROR\n";
    std::cout << "====================\n";
    std::cout << message << "\n";
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

void UI::showSuccess(const std::string& message) {
    system("cls");
    std::cout << "====================\n";
    std::cout << "  ÉXITO\n";
    std::cout << "====================\n";
    std::cout << message << "\n";
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

void UI::displayBoard(Board& board, bool hide_ships, bool is_opponent) {
    std::vector<std::string> ship_summary;
    for (const auto& ship : board.ship_instances) {
        std::string line = ship.name + " (" + std::to_string(ship.positions.size()) + "): ";
        line += ship.sunk ? "Hundido" : "A flote";
        ship_summary.push_back(line);
    }
    std::string header = is_opponent ? "Barcos del oponente:" : "Tus barcos:";
    ship_summary.insert(ship_summary.begin(), header);

    std::cout << "\n  ";
    for (int i = 0; i < BOARD_SIZE; i++) std::cout << " " << i << " ";
    std::cout << "   | ";
    if (!ship_summary.empty()) std::cout << ship_summary[0];
    std::cout << "\n +";
    for (int i = 0; i < BOARD_SIZE; i++) std::cout << "---";
    std::cout << "+  |";

    size_t ship_line = 1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        std::cout << "\n" << i << "|";
        for (int j = 0; j < BOARD_SIZE; j++) {
            char c = board.grid[i][j];
            if (hide_ships && c != HIT && c != MISS && c != WATER) c = WATER;
            if (c == HIT) {
                setConsoleColor(12); // Rojo
                std::cout << " X ";
                resetConsoleColor();
            } else if (c == MISS) {
                setConsoleColor(9); // Azul
                std::cout << " O ";
                resetConsoleColor();
            } else if (c == WATER) {
                setConsoleColor(11); // Cian
                std::cout << " ~ ";
                resetConsoleColor();
            } else {
                setConsoleColor(10); // Verde
                std::cout << " " << c << " ";
                resetConsoleColor();
            }
        }
        std::cout << "|  |";
        if (ship_line < ship_summary.size()) {
            std::cout << ship_summary[ship_line];
            ship_line++;
        }
    }
    std::cout << "\n +";
    for (int i = 0; i < BOARD_SIZE; i++) std::cout << "---";
    std::cout << "+  |";
    for (; ship_line < ship_summary.size(); ship_line++) {
        std::cout << "\n   " << std::string(BOARD_SIZE * 3, ' ') << "   |" << ship_summary[ship_line];
    }
    std::cout << "\n";
}

void UI::setupBoard(Board& board) {
    system("cls");
    std::cout << "==================================\n";
    std::cout << "  CONFIGURACIÓN DEL TABLERO\n";
    std::cout << "==================================\n";
    std::cout << "Desea colocar los barcos manualmente (M) o aleatoriamente (A)? ";
    std::string choice;
    std::getline(std::cin, choice);
    choice = toupper(choice.empty() ? 'A' : choice[0]);

    if (choice == "M") {
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                system("cls");
                displayBoard(board);
                std::cout << "Colocando " << ship.name << " (" << ship.size << " casillas)." << std::endl;

                std::cout << "Ingrese fila inicial (0-9): ";
                std::string input;
                std::getline(std::cin, input);

                try {
                    x = std::stoi(input);
                    if (x < 0 || x >= BOARD_SIZE) {
                        std::cout << "Fila inválida. Debe estar entre 0 y 9. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                        continue;
                    }
                } catch (const std::exception&) {
                    std::cout << "Entrada no numérica. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }

                std::cout << "Ingrese columna inicial (0-9): ";
                std::getline(std::cin, input);

                try {
                    y = std::stoi(input);
                    if (y < 0 || y >= BOARD_SIZE) {
                        std::cout << "Columna inválida. Debe estar entre 0 y 9. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                        continue;
                    }
                } catch (const std::exception&) {
                    std::cout << "Entrada no numérica. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }

                std::cout << "Ingrese dirección (H/V): ";
                std::getline(std::cin, input);
                if (input.empty()) {
                    std::cout << "Entrada vacía. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }
                direction = toupper(input[0]);

                if (direction == 'H' || direction == 'V') {
                    if (board.canPlaceShip(x, y, ship.size, direction)) {
                        board.placeShip(x, y, ship.size, direction, ship.symbol);
                        board.trackShip(ship, x, y, direction);
                        placed = true;
                    } else {
                        std::cout << "No se puede colocar el barco ahí. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                    }
                } else {
                    std::cout << "Dirección inválida. Use 'H' o 'V'. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                }
            }
        }
    } else {
        srand(time(0));
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                x = rand() % BOARD_SIZE;
                y = rand() % BOARD_SIZE;
                direction = (rand() % 2 == 0) ? 'H' : 'V';

                if (board.canPlaceShip(x, y, ship.size, direction)) {
                    board.placeShip(x, y, ship.size, direction, ship.symbol);
                    board.trackShip(ship, x, y, direction);
                    placed = true;
                }
            }
        }
    }

    system("cls");
    std::cout << "Tu tablero:\n";
    displayBoard(board);
    std::cout << "Tablero configurado. Presione Enter para continuar...";
    std::cin.get();
}

void UI::waitingAnimation(bool& esperando) {
    const char* anim[] = {"[ / ]", "[ - ]", "[ \\ ]", "[ | ]"};
    int i = 0;
    while (esperando) {
        std::cout << "\rBuscando oponente... " << anim[i % 4] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        i++;
    }
    std::cout << "\r                                    \r" << std::flush;
}