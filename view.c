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

// Semáforos
typedef struct {
    sem_t sem_A;
    sem_t sem_B;
    sem_t sem_C;
    sem_t sem_D;
    sem_t sem_E;
    unsigned int readers;
} GameSync;

int main() {
    int shm_fd = shm_open(SHM_STATE, O_RDWR, 0666);
    if(shm_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    //  saber el tamaño real:
    GameState temp_game;
    read(shm_fd, &temp_game, sizeof(GameState));
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    // Mapear el estado real:
    GameState *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    int shm_sync_fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open sync");
        exit(EXIT_FAILURE);
    }

    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap sync");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Esperar al máster
        sem_wait(&sync->sem_A);
        system("clear");

        // Imprimir el tablero con la nueva función
        printBoard(game);

        // Mostrar la info de los jugadores
        printf("\n\033[1;36mEstadísticas:\033[0m\n");
        for (unsigned int i = 0; i < game->num_players; i++) {
            printf("Jugador \033[1;32m%s\033[0m: \033[1;33m%d pts\033[0m (x=%d, y=%d)\n",
                   game->players[i].name,
                   game->players[i].score,
                   game->players[i].x,
                   game->players[i].y);
        }
        printf("\n");

        // Avisar que terminamos de imprimir
        sem_post(&sync->sem_B);
    }

    return 0;
}