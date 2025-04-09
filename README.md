# Servidor de Batalla Naval en AWS

## Descripción
Este proyecto implementa un servidor completo para el juego de Batalla Naval, desarrollado en C con sockets Berkeley y hilos POSIX. Desplegado en una instancia de AWS EC2, permite a los jugadores registrarse, entrar en cola, emparejarse, colocar barcos, disparar con un temporizador de 30 segundos por turno, y recibir actualizaciones del estado del juego. Todos los eventos se registran en un archivo de log. El servidor maneja desconexiones y errores de forma robusta.

### Características
- Registro y autenticación de jugadores (`REGISTER`, `LOGIN`, `LOG_OUT`).
- Cola de emparejamiento (`QUEUE`, `CANCEL_QUEUE`).
- Gestión de partidas (`MATCH`, `GAME`).
- Disparos con respuestas detalladas (`FIRE`, `HIT`, `MISS`, `SUNK`).
- Control de turnos con temporizador de 30 segundos (`TURN`).
- Lista de jugadores activos (`PLAYERS`).
- Registro de eventos en `Battleship-MP-Server/cmake-build-debug/log.log`.
- Despliegue en AWS con IP pública.

---

## Requisitos
- **Cliente**: Sistema con `netcat` (`nc`) instalado.
- **Servidor**: Instancia AWS EC2 (Ubuntu) con GCC, bibliotecas POSIX (`pthread`), y acceso SSH.
- **Red**: Puerto 8080 abierto en el Security Group de AWS.
