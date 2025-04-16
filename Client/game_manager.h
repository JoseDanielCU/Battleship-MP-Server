#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H
#include "game_common.h"
#include "network.h"
#include "ui.h"
#include <string>
#include <winsock2.h>

class GameManager {
public:
    static void startGame(SOCKET client_socket, const std::string& clientIP, const std::string& username);
    static bool loginMenu(SOCKET client_socket, std::string& username, const std::string& clientIP);
    static void searchGame(SOCKET client_socket, const std::string& clientIP, const std::string& username);
    static void menuLoggedIn(SOCKET client_socket, const std::string& clientIP, std::string& username);
};

#endif