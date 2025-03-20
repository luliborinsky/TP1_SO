#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "board.h"

#define SHM_STATE "/game_state"
#define SHM_SYNC  "/game_sync"

// Estructura de sincronización
typedef struct {
    sem_t sem_A; // Señal para la vista (hay cambios)
    sem_t sem_B; // Señal para el máster (vista terminó)
    sem_t sem_C; // Evita inanición del máster
    sem_t sem_D; // Mutex para el estado (escritor)
    sem_t sem_E; // Mutex para la variable readers
    unsigned int readers; // Cantidad de lectores
} GameSync;

int main() {
    // Abrir la memoria compartida del estado del juego
    int shm_fd = shm_open(SHM_STATE, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }

    // Leer el tamaño del estado del juego
    GameState temp_game;
    if (read(shm_fd, &temp_game, sizeof(GameState)) == -1) {
        perror("read game_state");
        exit(EXIT_FAILURE);
    }
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    // Mapear la memoria compartida del estado del juego
    GameState *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap game_state");
        exit(EXIT_FAILURE);
    }

    // Abrir la memoria compartida de sincronización
    int shm_sync_fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open game_sync");
        exit(EXIT_FAILURE);
    }

    // Mapear la memoria compartida de sincronización
    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap game_sync");
        exit(EXIT_FAILURE);
    }

    // Bucle principal de la vista
    while (1) {
        // Esperar a que el máster indique que hay cambios
        sem_wait(&sync->sem_A);

        // Limpiar la pantalla
        system("clear");

        // Imprimir el tablero
        printBoard(game);

        // Mostrar estadísticas de los jugadores
        printf("\n\033[1;36mEstadísticas:\033[0m\n");
        for (unsigned int i = 0; i < game->num_players; i++) {
            printf("Jugador \033[1;32m%s\033[0m: \033[1;33m%d pts\033[0m (x=%d, y=%d)\n",
                   game->players[i].name,
                   game->players[i].score,
                   game->players[i].x,
                   game->players[i].y);
        }
        printf("\n");

        // Notificar al máster que la vista terminó de imprimir
        sem_post(&sync->sem_B);
    }

    return 0;
}