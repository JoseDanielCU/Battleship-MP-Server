#pragma once

// Incluye la definición del estado del servidor.
#include "server_state.h"

// Inicia una nueva partida entre dos jugadores.
void start_game(const std::string& player1, const std::string& player2, ServerState& state);
// Gestiona el emparejamiento de jugadores en la cola.
void matchmaking(ServerState& state);