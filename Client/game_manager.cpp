#include "game_manager.h" // Incluye el encabezado de la clase GameManager
#include <iostream> // Incluye la biblioteca para entrada/salida
#include <sstream> // Incluye la biblioteca para manipulación de cadenas
#include <thread> // Incluye la biblioteca para hilos
#include <chrono> // Incluye la biblioteca para mediciones de tiempo
#include "ui.h" // Incluye funciones de interfaz de usuario
#include "network.h" // Incluye funciones de red

// Inicia una partida de Battleship
void GameManager::startGame(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    // Crea tableros para el jugador y el oponente
    Board player_board, opponent_board;
    // Configura el tablero del jugador (manual o aleatoriamente)
    UI::setupBoard(player_board);

    // Vector para almacenar los últimos eventos del juego
    std::vector<std::string> event_log;
    // Lambda para agregar un evento al log
    auto add_event = [&event_log](const std::string& event) {
        // Agrega el evento al vector
        event_log.push_back(event);
        // Mantiene solo los últimos 5 eventos
        if (event_log.size() > 5) event_log.erase(event_log.begin());
    };

    // Serializa el tablero del jugador para enviarlo
    std::string board_data = player_board.serialize();
    // Envía el tablero al servidor
    // Protocolo: "BOARD|data" envía el estado del tablero
    std::string response = Network::sendMessage(client_socket, clientIP, "BOARD|" + board_data);
    // Verifica si la conexión se perdió
    if (response == "ERROR|CONNECTION_LOST") {
        // Muestra un mensaje de error
        UI::showError("Conexión perdida al enviar el tablero.");
        return;
    }
    // Agrega un evento al log
    add_event("Tablero enviado al servidor.");

    // Muestra un mensaje de espera
    std::cout << "Tablero enviado. Esperando inicio del juego...\n";

    // Buffer para recibir mensajes del servidor
    char buffer[1024] = {0};
    // Recibe el mensaje inicial del servidor
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    // Verifica si la recepción falló
    if (bytes_received <= 0) {
        // Registra un error de conexión
        Network::logEvent(clientIP, "INITIAL_TURN", "ERROR|CONNECTION_LOST");
        // Muestra un mensaje de error
        UI::showError("Conexión perdida al esperar el turno inicial.");
        return;
    }
    // Asegura que el buffer termine en null
    buffer[bytes_received] = '\0';
    // Convierte el buffer a una cadena
    std::string initial_message(buffer);
    // Registra el evento en el log
    Network::logEvent(clientIP, "INITIAL_TURN", initial_message);
    // Procesa el mensaje inicial
    std::istringstream iss(initial_message);
    std::string command, arg1, arg2;
    // Extrae el comando y los argumentos
    std::getline(iss, command, '|');
    std::getline(iss, arg1, '|');
    std::getline(iss, arg2, '|');

    // Bandera para indicar si el juego está activo
    bool game_active = true;
    // Bandera para indicar si es el turno del jugador
    bool my_turn = false;
    // Verifica si el mensaje inicial es un comando TURN
    if (command == "TURN") {
        // Determina si es el turno del jugador
        my_turn = (arg1 == username);
        // Limpia la consola
        system("cls");
        // Muestra el encabezado del juego
        std::cout << "==================================\n";
        // Indica de quién es el turno
        std::cout << (my_turn ? "  TU TURNO" : "  TURNO DEL OPONENTE") << "\n";
        std::cout << "==================================\n";
        // Muestra el tablero del jugador
        std::cout << "Tu tablero:\n";
        UI::displayBoard(player_board, false, false);
        // Muestra el tablero del oponente
        std::cout << "\nTablero del oponente:\n";
        UI::displayBoard(opponent_board, true, true);
        // Muestra los últimos eventos
        std::cout << "\nÚltimos eventos:\n";
        for (const auto& event : event_log) {
            std::cout << "- " << event << "\n";
        }
        std::cout << "==================================\n";
    } else {
        // Muestra un mensaje de error si el comando no es el esperado
        UI::showError("No se recibió el turno inicial esperado. Mensaje: " + initial_message);
        return;
    }

    // Configura el socket en modo no bloqueante
    u_long mode = 1;
    ioctlsocket(client_socket, FIONBIO, &mode);

    // Bandera para evitar repetir el mensaje de espera
    bool waiting_message_displayed = false;

    // Bucle principal del juego
    while (game_active) {
        // Bandera para indicar si se recibió un mensaje
        bool received_message = false;
        do {
            // Intenta recibir un mensaje del servidor
            bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            // Verifica si ocurrió un error
            if (bytes_received == SOCKET_ERROR) {
                // Si el error es porque no hay datos disponibles, sale del bucle
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    break;
                } else {
                    // Registra un error de conexión
                    Network::logEvent(clientIP, "GAME_LISTEN", "ERROR|CONNECTION_LOST");
                    // Muestra un mensaje de error
                    UI::showError("Conexión perdida durante el juego.");
                    // Termina el juego
                    game_active = false;
                    break;
                }
            } else if (bytes_received > 0) {
                // Asegura que el buffer termine en null
                buffer[bytes_received] = '\0';
                // Convierte el buffer a una cadena
                std::string message(buffer);
                // Registra el mensaje recibido
                Network::logEvent(clientIP, "RECEIVED", message);

                // Procesa el mensaje
                std::istringstream iss(message);
                std::getline(iss, command, '|');
                std::getline(iss, arg1, '|');
                std::getline(iss, arg2, '|');

                // Indica que se recibió un mensaje
                received_message = true;
                // Resetea la bandera de mensaje de espera
                waiting_message_displayed = false;

                // Maneja el comando TURN
                if (command == "TURN") {
                    // Actualiza la bandera de turno
                    my_turn = (arg1 == username);
                } else if (command == "ATTACKED") {
                    // El oponente atacó una posición
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    // Verifica si la casilla contiene un barco
                    if (player_board.grid[x][y] != WATER && player_board.grid[x][y] != HIT && player_board.grid[x][y] != MISS) {
                        // Marca la casilla como acertada
                        player_board.grid[x][y] = HIT;
                        // Actualiza el estado de hundimiento
                        player_board.updateSunkStatus(x, y);
                        // Agrega un evento al log
                        add_event("Oponente acertó en (" + std::to_string(x) + "," + std::to_string(y) + ").");
                    } else {
                        // Marca la casilla como fallida
                        player_board.grid[x][y] = MISS;
                        // Agrega un evento al log
                        add_event("Oponente falló en (" + std::to_string(x) + "," + std::to_string(y) + ").");
                    }
                } else if (command == "HIT") {
                    // El jugador acertó un disparo
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    // Marca la casilla como acertada en el tablero del oponente
                    opponent_board.grid[x][y] = HIT;
                    // Actualiza el estado de hundimiento
                    opponent_board.updateSunkStatus(x, y);
                    // Agrega un evento al log
                    add_event("¡Acertaste en (" + std::to_string(x) + "," + std::to_string(y) + ")!");
                } else if (command == "MISS") {
                    // El jugador falló un disparo
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    // Marca la casilla como fallida en el tablero del oponente
                    opponent_board.grid[x][y] = MISS;
                    // Agrega un evento al log
                    add_event("Fallaste en (" + std::to_string(x) + "," + std::to_string(y) + ").");
                } else if (command == "SUNK") {
                    // Un barco del oponente fue hundido
                    for (auto& ship : opponent_board.ship_instances) {
                        if (ship.name == arg1 && !ship.sunk) {
                            // Marca el barco como hundido
                            ship.sunk = true;
                            break;
                        }
                    }
                    // Agrega un evento al log
                    add_event("¡Hundiste un " + arg1 + "!");
                } else if (command == "GAME") {
                    // Maneja el fin del juego
                    system("cls");
                    if (arg1 == "WIN") {
                        // Agrega un evento de victoria
                        add_event("¡Ganaste la partida!");
                        // Muestra un mensaje de victoria
                        std::cout << "==================================\n";
                        std::cout << "  ¡GANASTE LA PARTIDA!\n";
                        std::cout << "==================================\n";
                    } else if (arg1 == "LOSE") {
                        // Agrega un evento de derrota
                        add_event("Perdiste la partida.");
                        // Muestra un mensaje de derrota
                        std::cout << "==================================\n";
                        std::cout << "  ¡Perdiste! El oponente ganó.\n";
                        std::cout << "==================================\n";
                    } else if (arg1 == "ENDED") {
                        // Agrega un evento de finalización
                        add_event("Partida finalizada.");
                        // Muestra un mensaje de finalización
                        std::cout << "==================================\n";
                        std::cout << "  Partida finalizada.\n";
                        std::cout << "==================================\n";
                    }
                    // Solicita al usuario presionar Enter
                    std::cout << "Presione Enter para volver al menú principal...";
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cin.get();
                    // Termina el juego
                    game_active = false;
                    break;
                }

                // Actualiza la interfaz
                system("cls");
                std::cout << "==================================\n";
                std::cout << (my_turn ? "  TU TURNO" : "  TURNO DEL OPONENTE") << "\n";
                std::cout << "==================================\n";
                std::cout << "Tu tablero:\n";
                UI::displayBoard(player_board, false, false);
                std::cout << "\nTablero del oponente:\n";
                UI::displayBoard(opponent_board, true, true);
                std::cout << "\nÚltimos eventos:\n";
                for (const auto& event : event_log) {
                    std::cout << "- " << event << "\n";
                }
                std::cout << "==================================\n";
            }
        } while (bytes_received > 0 && game_active);

        // Si es el turno del jugador
        if (my_turn) {
            // Solicita coordenadas para disparar
            std::cout << "Ingresa coordenadas para disparar (fila columna, ej: 3 4): ";
            std::string input;
            // Lee la entrada del usuario
            if (std::getline(std::cin, input)) {
                std::istringstream iss(input);
                int x, y;
                // Verifica si las coordenadas son válidas
                if (iss >> x >> y && x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && opponent_board.grid[x][y] != HIT && opponent_board.grid[x][y] != MISS) {
                    // Crea el mensaje de disparo
                    // Protocolo: "FIRE|x|y" indica un disparo a las coordenadas (x, y)
                    std::string fire_msg = "FIRE|" + std::to_string(x) + "|" + std::to_string(y);
                    // Envía el mensaje al servidor
                    int result = send(client_socket, fire_msg.c_str(), fire_msg.size(), 0);
                    // Verifica si el envío falló
                    if (result == SOCKET_ERROR) {
                        // Registra un error de envío
                        Network::logEvent(clientIP, "FIRE", "ERROR|SEND_FAILED: " + std::to_string(WSAGetLastError()));
                        // Muestra un mensaje de error
                        UI::showError("Error al enviar disparo al servidor.");
                    } else {
                        // Registra el evento de disparo
                        Network::logEvent(clientIP, "FIRE", fire_msg);
                        // Pasa el turno al oponente
                        my_turn = false;
                    }
                } else {
                    // Muestra un mensaje si las coordenadas son inválidas
                    std::cout << "Coordenadas inválidas o ya disparaste ahí. Intenta de nuevo.\n";
                }
            }
        } else if (!waiting_message_displayed && !received_message && game_active) {
            // Muestra un mensaje de espera
            std::cout << "Esperando el turno del oponente...\n";
            // Evita repetir el mensaje
            waiting_message_displayed = true;
        }

        // Pausa breve para evitar consumo excesivo de CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Restaura el socket a modo bloqueante
    mode = 0;
    ioctlsocket(client_socket, FIONBIO, &mode);

    // Muestra un mensaje para volver al menú
    std::cout << "Volviendo al menú principal... Presione Enter.";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

// Muestra el menú de login y maneja la autenticación
bool GameManager::loginMenu(SOCKET client_socket, std::string& username, const std::string& clientIP) {
    // Bucle para el menú de login
    while (true) {
        // Muestra el menú principal
        UI::drawMenu("MENÚ PRINCIPAL", {"Registrarse", "Iniciar sesión", "Salir"});
        // Lee la opción del usuario
        std::string input;
        std::getline(std::cin, input);

        // Si el usuario elige salir
        if (input == "3") return false;

        // Verifica si la opción es válida
        if (input != "1" && input != "2") {
            // Muestra un mensaje de error
            UI::showError("Opción no válida.");
            continue;
        }

        // Limpia la consola
        system("cls");
        // Muestra el encabezado según la opción
        std::cout << "====================\n";
        std::cout << "  " << (input == "1" ? "REGISTRO" : "INICIO DE SESIÓN") << "\n";
        std::cout << "====================\n";
        // Solicita el nombre de usuario
        std::cout << "Usuario: ";
        std::getline(std::cin, username);
        // Solicita la contraseña
        std::cout << "Contraseña: ";
        std::string password;
        std::getline(std::cin, password);

        // Verifica que los campos no estén vacíos ni contengan '|'
        if (username.empty() || password.empty() || username.find('|') != std::string::npos || password.find('|') != std::string::npos) {
            // Muestra un mensaje de error
            UI::showError("Usuario y contraseña no pueden estar vacíos ni contener '|'.");
            continue;
        }

        // Crea el mensaje según la opción
        // Protocolo: "REGISTER|username|password" o "LOGIN|username|password"
        std::string query = (input == "1") ? "REGISTER|" + username + "|" + password : "LOGIN|" + username + "|" + password;
        try {
            // Envía el mensaje al servidor
            std::string response = Network::sendMessage(client_socket, clientIP, query);
            // Maneja la respuesta del servidor
            if (response == "LOGIN|SUCCESSFUL") {
                // Muestra un mensaje de éxito
                UI::showSuccess("¡Bienvenido, " + username + "!");
                // Indica que el login fue exitoso
                return true;
            } else if (response == "LOGIN|ERROR|YA_CONECTADO") {
                // Muestra un mensaje de error
                UI::showError("Este usuario ya está conectado.");
            } else if (response == "LOGIN|ERROR") {
                // Muestra un mensaje de error
                UI::showError("Usuario o contraseña incorrectos.");
            } else if (response == "REGISTER|SUCCESSFUL") {
                // Muestra un mensaje de éxito
                UI::showSuccess("¡Usuario " + username + " registrado con éxito!");
            } else if (response == "REGISTER|ERROR") {
                // Muestra un mensaje de error
                UI::showError("El usuario ya existe.");
            } else {
                // Muestra un mensaje de error desconocido
                UI::showError("Error desconocido: " + response);
            }
        } catch (const std::runtime_error& e) {
            // Muestra un mensaje de error
            UI::showError(e.what());
            // Termina el menú
            return false;
        }
    }
}

// Busca una partida en el servidor
void GameManager::searchGame(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    // Envía una solicitud para entrar en la cola
    // Protocolo: "QUEUE|username" indica que el usuario quiere jugar
    std::string response = Network::sendMessage(client_socket, clientIP, "QUEUE|" + username);
    // Verifica si la solicitud fue aceptada
    if (response != "QUEUE|OK") {
        // Muestra un mensaje de error
        UI::showError("Error al entrar en cola: " + response);
        return;
    }

    // Bandera para indicar que está en la cola
    bool queue = true;
    // Bandera para indicar si se encontró un oponente
    bool match_found = false;

    // Inicia un hilo para mostrar una animación de espera
    std::thread animation(UI::waitingAnimation, std::ref(queue));
    // Registra el tiempo de inicio
    auto start_time = std::chrono::steady_clock::now();

    // Bucle para verificar si se encuentra un oponente
    while (queue) {
        // Espera 1 segundo entre verificaciones
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // Calcula el tiempo transcurrido
        auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        // Si han pasado 15 segundos, sale de la cola
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed_time).count() >= 15) {
            queue = false;
            // Muestra un mensaje de salida
            std::cout << "\nNo se encontró oponente, saliendo de la cola..." << std::endl;
            // Envía una solicitud para cancelar la cola
            // Protocolo: "CANCEL_QUEUE|username"
            Network::sendMessage(client_socket, clientIP, "CANCEL_QUEUE|" + username);
            break;
        }

        // Verifica si se encontró un oponente
        // Protocolo: "CHECK_MATCH|username"
        response = Network::sendMessage(client_socket, clientIP, "CHECK_MATCH|" + username);
        if (response.find("MATCH|FOUND") != std::string::npos) {
            // Indica que se encontró un oponente
            queue = false;
            match_found = true;
            // Muestra un mensaje de éxito
            std::cout << "\n¡Oponente encontrado! La partida comenzará." << std::endl;
            break;
        }
    }

    // Une el hilo de la animación si está activo
    if (animation.joinable()) {
        animation.join();
    }

    // Si se encontró un oponente, inicia la partida
    if (match_found) {
        startGame(client_socket, clientIP, username);
    }
}

// Muestra el menú para usuarios autenticados
void GameManager::menuLoggedIn(SOCKET client_socket, const std::string& clientIP, std::string& username) {
    // Bucle para el menú de juego
    while (true) {
        // Muestra el menú con las opciones disponibles
        UI::drawMenu("MENÚ DE JUEGO - " + username, {
            "Ver jugadores conectados",
            "Entrar en Cola",
            "Cerrar Sesión"
        });
        // Lee la opción del usuario
        std::string input;
        std::getline(std::cin, input);

        // Opción: Ver jugadores conectados
        if (input == "1") {
            try {
                // Envía una solicitud para listar jugadores
                // Protocolo: "PLAYERS|"
                std::string response = Network::sendMessage(client_socket, clientIP, "PLAYERS|");
                // Verifica si la conexión se perdió
                if (response == "ERROR|CONNECTION_LOST") {
                    // Muestra un mensaje de error
                    UI::showError("Conexión perdida con el servidor al intentar listar jugadores. Volviendo al menú principal...");
                    return;
                }
                // Limpia la consola
                system("cls");
                // Muestra la lista de jugadores
                std::cout << "====================\n";
                std::cout << "  JUGADORES CONECTADOS\n";
                std::cout << "====================\n";
                std::cout << response << "\n";
                std::cout << "Presione Enter para continuar...";
                std::cin.get();
            } catch (const std::runtime_error& e) {
                // Muestra un mensaje de error
                UI::showError(std::string(e.what()) + " Volviendo al menú principal...");
                return;
            }
        } else if (input == "2") {
            // Opción: Entrar en cola
            searchGame(client_socket, clientIP, username);
        } else if (input == "3") {
            // Opción: Cerrar sesión
            // Envía un mensaje de LOGOUT
            // Protocolo: "LOGOUT|username"
            std::string response = Network::sendMessage(client_socket, clientIP, "LOGOUT|" + username);
            // Verifica si la conexión se perdió
            if (response == "ERROR|CONNECTION_LOST") {
                // Muestra un mensaje de error
                UI::showError("Conexión perdida al intentar cerrar sesión. Volviendo al menú principal...");
            }
            // Sale del menú
            break;
        } else {
            // Muestra un mensaje para opciones inválidas
            std::cout << "Opción no válida. Presione Enter para continuar...";
            std::cin.get();
        }
    }
}