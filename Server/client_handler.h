#pragma once

// Incluye la definición del estado del servidor.
#include "server_state.h"

// Maneja la comunicación con un cliente conectado.
void handle_client(int client_socket, ServerState& state);