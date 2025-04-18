# Battleship - Batalla Naval Cliente-Servidor

## Descripción
Battleship es un juego multijugador en red basado en el clásico juego de Batalla Naval, implementado en C++ con una arquitectura cliente-servidor. Los jugadores pueden registrarse, iniciar sesión, configurar tableros de juego (manualmente o aleatoriamente), y competir en partidas 1v1 en un tablero de 10x10. El proyecto soporta dos versiones del servidor: una para Windows (usando Winsock) y otra para Linux (usando sockets POSIX).

## Características
- **Autenticación**: Registro e inicio de sesión de usuarios.
- **Configuración del tablero**: Manual o aleatoria para colocar barcos.
- **Juego en red**: Partidas 1v1 con turnos alternados.
- **Interfaz de consola**: Visualización de tableros, eventos y menús.
- **Soporte multiplataforma**: Servidor compatible con Windows y Linux.
- **Registro de eventos**: Logs para depuración y monitoreo.

## Requisitos
### Cliente (Windows)
- Sistema operativo: Windows 10 o superior.
- Compilador: MSVC (Visual Studio) o MinGW con soporte para C++17.
- Bibliotecas: Winsock2 (`ws2_32.lib`).

### Servidor
- **Windows**: Igual que el cliente.
- **Linux**: Distribución con soporte para g++ y bibliotecas estándar POSIX.
- Compilador: g++ con soporte para C++17.
