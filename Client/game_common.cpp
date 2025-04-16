#include "game_common.h"

std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'},
    {"Battleship", 4, 'B'},
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'},
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'},
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}
};

Board::Board() {
    grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
}

bool Board::canPlaceShip(int x, int y, int size, char direction) {
    if (direction == 'H') {
        if (y + size > BOARD_SIZE) return false;
        for (int i = 0; i < size; i++)
            if (grid[x][y + i] != WATER) return false;
    } else {
        if (x + size > BOARD_SIZE) return false;
        for (int i = 0; i < size; i++)
            if (grid[x + i][y] != WATER) return false;
    }
    return true;
}

void Board::placeShip(int x, int y, int size, char direction, char symbol) {
    for (int i = 0; i < size; i++) {
        if (direction == 'H')
            grid[x][y + i] = symbol;
        else
            grid[x + i][y] = symbol;
    }
}

void Board::trackShip(const Ship& ship, int x, int y, char direction) {
    std::vector<std::pair<int, int>> positions;
    for (int i = 0; i < ship.size; i++) {
        int pos_x = (direction == 'H') ? x : x + i;
        int pos_y = (direction == 'H') ? y + i : y;
        positions.emplace_back(pos_x, pos_y);
    }
    ship_instances.emplace_back(ship.name, ship.symbol, positions);
}

void Board::updateSunkStatus(int x, int y) {
    for (auto& ship : ship_instances) {
        if (!ship.sunk) {
            for (const auto& pos : ship.positions) {
                if (pos.first == x && pos.second == y) {
                    bool all_hit = true;
                    for (const auto& p : ship.positions) {
                        if (grid[p.first][p.second] != HIT) {
                            all_hit = false;
                            break;
                        }
                    }
                    if (all_hit) {
                        ship.sunk = true;
                    }
                    break;
                }
            }
        }
    }
}

std::string Board::serialize() {
    std::string data;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            data += grid[i][j];
        }
    }
    return data;
}