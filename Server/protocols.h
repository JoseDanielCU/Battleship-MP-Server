#pragma once

// Incluye la biblioteca para manipulación de cadenas, como std::string.
#include <string>
// Incluye la definición del estado del servidor.
#include "server_state.h"

// Procesa los comandos recibidos desde un cliente.
void process_protocols(const std::string& command, const std::string& param1, const std::string& param2, int client_socket, std::string& logged_user, ServerState& state);