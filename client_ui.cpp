#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <windows.h>
#include "game_common.h"

void draw_menu(const std::string& title, const std::vector<std::string>& options) {
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

void show_error(const std::string& message) {
    system("cls");
    std::cout << "====================\n";
    std::cout << "  ERROR\n";
    std::cout << "====================\n";
    std::cout << message << "\n";
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

void show_success(const std::string& message) {
    system("cls");
    std::cout << "====================\n";
    std::cout << "  ÉXITO\n";
    std::cout << "====================\n";
    std::cout << message << "\n";
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

void waiting_animation(bool& esperando) {
    const char* anim[] = {"[ / ]", "[ - ]", "[ \\ ]", "[ | ]"};
    int i = 0;
    while (esperando) {
        std::cout << "\rBuscando oponente... " << anim[i % 4] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        i++;
    }
    std::cout << "\r                                    \r" << std::flush;
}

void setup_board(Board& board) {
    system("cls");
    std::cout << "==================================\n";
    std::cout << "  CONFIGURACIÓN DEL TABLERO\n";
    std::cout << "==================================\n";
    std::cout << "Desea colocar los barcos manualmente (M) o aleatoriamente (A)? ";
    std::string choice;
    std::getline(std::cin, choice);
    choice = toupper(choice[0]);

    if (choice == "M") {
        board.manualPlacement();
    } else {
        board.randomPlacement();
    }

    system("cls");
    std::cout << "Tu tablero:\n";
    board.display();
    std::cout << "Tablero configurado. Presione Enter para continuar...";
    std::cin.get();
}