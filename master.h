#ifndef MASTER_H
#define MASTER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

typedef struct Player{
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int inv_moves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int v_moves; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y; // Coordenadas x e y en el tablero
    pid_t pid; // Identificador de proceso
    bool is_blocked; // Indica si el jugador tiene movimientos bloqueados
} Player;

typedef struct{
    unsigned short width, height; // dimensioens
    unsigned int num_players; // cantidad de jugadores
    Player players[9]; // lista de jugadores
    bool game_over; // si el juego termino o no
    int board[]; // tablero dinamico
}GameState;

typedef struct {
    sem_t print_needed; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t print_done; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t master_utd; // Mutex para evitar inanición del master al acceder al estado
    sem_t game_state_change; // Mutex para el estado del juego
    sem_t sig_var; // Mutex para la siguiente variable
    unsigned int readers; // Cantidad de jugadores leyendo el estado
} GameSync;


#endif 