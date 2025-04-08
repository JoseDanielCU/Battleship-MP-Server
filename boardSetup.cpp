#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

const int BOARD_SIZE = 10;
const char WATER = '~';


struct Ship {
    string name;
    int size;
    char symbol;
};

// List of ships
vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'},
    {"Battleship", 4, 'B'},
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'},
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'},
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}
};

// Class representing the game board
class Board {
public:
    vector<vector<char>> grid;  // Board grid

    // Constructor: initializes the board with water
    Board() {
        grid.resize(BOARD_SIZE, vector<char>(BOARD_SIZE, WATER));
    }

    // Method to display the board in the terminal
    void display() {
        cout << "   ";
        for (int i = 0; i < BOARD_SIZE; i++) cout << i << "  ";
        cout << endl;

        for (int i = 0; i < BOARD_SIZE; i++) {
            cout << i << "  ";
            for (int j = 0; j < BOARD_SIZE; j++) {
                cout << grid[i][j] << "  ";
            }
            cout << endl;
        }
    }

    // Method to check if a ship can be placed at a specific position
    bool canPlaceShip(int x, int y, int size, char direction) {
        if (direction == 'H') {  // Horizontal placement
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

    // Method to place a ship on the board
    void placeShip(int x, int y, int size, char direction, char symbol) {
        for (int i = 0; i < size; i++) {
            if (direction == 'H')
                grid[x][y + i] = symbol;
            else
                grid[x + i][y] = symbol;
        }
    }

    // Method to place a ships manual
    void manualPlacement() {
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                display();
                cout << "Colocando " << ship.name << " (" << ship.size << " casillas)." << endl;
                cout << "Ingrese fila inicial: "; cin >> x;

                cout << "Ingrese columna inicial: "; cin >> y;
                cout << "Ingrese direccion (H/V): "; cin >> direction;
                direction = toupper(direction);

                if (canPlaceShip(x, y, ship.size, direction)) {
                    placeShip(x, y, ship.size, direction, ship.symbol);
                    placed = true;
                } else {
                    cout << "======================================================" << endl;
                    cout << "El barco no puede ir en esa posición. Intente de nuevo." << endl;
                    cout << "======================================================" << endl;
                }
            }
        }
    }


    // Method to randomly place ships
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
};

// main
int main() {
    Board playerBoard;
    char choice;

    cout << "Desea colocar los barcos manualmente (M) o aleatoriamente (A)? ";
    cin >> choice;
    choice = toupper(choice);

    if (choice == 'M') {
        playerBoard.manualPlacement();
    } else {
        playerBoard.randomPlacement();
    }

    cout << "Tablero final:" << endl;
    playerBoard.display();
    return 0;
}