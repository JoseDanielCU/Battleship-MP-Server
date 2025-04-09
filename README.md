# Battleship-MP-Server

## Descripción
Este proyecto es un servidor de Batalla Naval implementado en C++ utilizando Winsock para la comunicación en red e hilos STL para concurrencia. Desplegado en una instancia AWS EC2 (Windows), permite a dos jugadores registrarse, iniciar sesión, entrar en cola, configurar tableros, jugar turnos, y manejar desconexiones. Los eventos se registran en un archivo de log. El servidor soporta una flota predefinida de 9 barcos en un tablero de 10x10.

### Características
- Registro y autenticación (`REGISTER`, `LOGIN`, `LOGOUT`).
- Cola de emparejamiento (`QUEUE`, `CANCEL_QUEUE`).
- Configuración de tableros mediante `BOARD`.
- Juego por turnos con disparos (`FIRE`, `HIT`, `MISS`, `SUNK`).
- Lista de jugadores activos (`PLAYERS`).
- Registro de eventos en un archivo de log.
- Despliegue en AWS con IP pública.

### Flota de Barcos
- Aircraft Carrier 
- Battleship 
- 2x Cruiser 
- 2x Destroyer 
- 3x Submarine 

---

## Requisitos
- **Cliente**: Sistema con `netcat` (`nc`) instalado.
- **Servidor**: AWS EC2 (Windows Server) con MinGW o MSVC, y Winsock (`ws2_32.lib`).
- **Red**: Puerto 8080 abierto en el Security Group de AWS.

---
