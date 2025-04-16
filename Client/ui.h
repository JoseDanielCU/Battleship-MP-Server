#ifndef UI_H
#define UI_H
#include "game_common.h"
#include <string>
#include <vector>
#include <windows.h>

class UI {
public:
    static void setConsoleColor(int color);
    static void resetConsoleColor();
    static void drawMenu(const std::string& title, const std::vector<std::string>& options);
    static void showError(const std::string& message);
    static void showSuccess(const std::string& message);
    static void displayBoard(Board& board, bool hide_ships = false, bool is_opponent = false);
    static void setupBoard(Board& board);
    static void waitingAnimation(bool& esperando);
};

#endif