#ifndef GAME_COMMON_H
#define GAME_COMMON_H

#include <vector>
#include <string>
#include <map>

#define BOARD_SIZE 10
#define WATER '~'
#define HIT 'X'
#define MISS 'O'

struct Ship {
    std::string name;
    int size;
    char symbol;
};

extern std::vector<Ship> ships;

class Board {
public:
    std::vector<std::vector<char>> grid;
    struct ShipInstance {
        std::string name;
        char symbol;
        std::vector<std::pair<int, int>> positions;
        bool sunk;
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
            : name(n), symbol(s), positions(pos), sunk(false) {}
    };
    std::vector<ShipInstance> ship_instances;

    Board() {
        grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
    }

    void display(bool hide_ships = false) {
        std::cout << "   ";
        for (int i = 0; i < BOARD_SIZE; i++) std::cout << i << "  ";
        std::cout << std::endl;

        for (int i = 0; i < BOARD_SIZE; i++) {
            std::cout << i << "  ";
            for (int j = 0; j < BOARD_SIZE; j++) {
                char c = grid[i][j];
                if (hide_ships && c != HIT && c != MISS && c != WATER) c = WATER;
                std::cout << c << "  ";
            }
            std::cout << std::endl;
        }
    }

    bool canPlaceShip(int x, int y, int size, char direction) {
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

    void placeShip(int x, int y, int size, char direction, char symbol) {
        for (int i = 0; i < size; i++) {
            if (direction == 'H')
                grid[x][y + i] = symbol;
            else
                grid[x + i][y] = symbol;
        }
    }

    void manualPlacement() {
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                system("cls");
                display();
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
                    if (canPlaceShip(x, y, ship.size, direction)) {
                        placeShip(x, y, ship.size, direction, ship.symbol);
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
    }

    void randomPlacement() {
        srand(time(0));
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                x = rand() % BOARD_SIZE;
                y = rand() % BOARD_SIZE;
                direction = (rand() % 2 == 0) ? 'H' : 'V';

                if (canPlaceShip(x, y, ship.size, direction)) {
                    placeShip(x, y, ship.size, direction, ship.symbol);
                    placed = true;
                }
            }
        }
    }

    std::string serialize() {
        std::string data;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                data += grid[i][j];
            }
        }
        return data;
    }

    void deserialize(const std::string& data) {
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                grid[i][j] = data[i * BOARD_SIZE + j];
            }
        }
        rebuildShipInstances();
    }

    void rebuildShipInstances() {
        ship_instances.clear();
        std::map<char, std::vector<std::pair<int, int>>> symbol_to_positions;

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                    symbol_to_positions[grid[i][j]].emplace_back(i, j);
                }
            }
        }

        for (const auto& ship : ships) {
            auto& positions = symbol_to_positions[ship.symbol];
            int size = ship.size;

            while (positions.size() >= size) {
                std::vector<std::pair<int, int>> current_ship;
                for (auto it = positions.begin(); it != positions.end(); ) {
                    if (current_ship.empty()) {
                        current_ship.push_back(*it);
                        it = positions.erase(it);
                    } else {
                        bool adjacent = false;
                        auto last = current_ship.back();
                        for (auto pos_it = it; pos_it != positions.end(); ++pos_it) {
                            if ((pos_it->first == last.first && abs(pos_it->second - last.second) == 1) ||
                                (pos_it->second == last.second && abs(pos_it->first - last.first) == 1)) {
                                current_ship.push_back(*pos_it);
                                it = positions.erase(pos_it);
                                adjacent = true;
                                break;
                            }
                        }
                        if (!adjacent) break;
                    }
                    if (current_ship.size() == size) break;
                }
                if (current_ship.size() == size) {
                    ship_instances.emplace_back(ship.name, ship.symbol, current_ship);
                } else {
                    positions.insert(positions.begin(), current_ship.begin(), current_ship.end());
                    break;
                }
            }
        }
    }

    bool isHit(int x, int y) const {
        return grid[x][y] != WATER && grid[x][y] != HIT && grid[x][y] != MISS;
    }

    char getSymbol(int x, int y) const {
        return grid[x][y];
    }

    void setHit(int x, int y) {
        grid[x][y] = HIT;
        checkSunk(x, y);
    }

    void setMiss(int x, int y) {
        grid[x][y] = MISS;
    }

    void checkSunk(int x, int y) {
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
                        return;
                    }
                }
            }
        }
    }

    std::string getSunkShipName(int x, int y) {
        for (const auto& ship : ship_instances) {
            if (ship.sunk) {
                for (const auto& pos : ship.positions) {
                    if (pos.first == x && pos.second == y) {
                        return ship.name;
                    }
                }
            }
        }
        return "";
    }

    bool allShipsSunk() {
        for (const auto& ship : ship_instances) {
            if (!ship.sunk) return false;
        }
        return true;
    }

    int countShips() const {
        int total_length = 0;
        for (const auto& ship : ships) {
            total_length += ship.size;
        }
        int current_length = 0;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                    current_length++;
                }
            }
        }
        return (current_length == total_length) ? total_length : 0;
    }
};

#endif