// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "master.h"

#define SHM_game "/game_game"
#define SHM_SYNC "/game_sync"
#define TRUE 1
#define FALSE 0


int main() {
    int game_game_fd = shm_open("/game_game", O_RDONLY, 0644);
    if(game_game_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Leer valores básicos para saber el tamaño real:
    Gamegame temp_game;
    read(game_game_fd, &temp_game, sizeof(Gamegame));
    size_t game_size = sizeof(Gamegame) + temp_game.width * temp_game.height * sizeof(int);

    // Mapear el estado real:
    Gamegame *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, game_game_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    int shm_sync_fd = shm_open("/game_sync", O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open sync");
        exit(EXIT_FAILURE);
    }

    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap sync");
        exit(EXIT_FAILURE);
    }

    while (!game->game_over) {
        sem_wait(&(sync->print_needed));
        
        // Imprimir el tablero
        for (int i = 0; i < game->height; i++) {
            for (int j = 0; j < game->width; j++) {
                int cell = game->board[i * game->width + j];
                if (cell > 0) {
                    // Recompensa
                    printf("%d ", cell); 
                } else {
                    // Cabeza y cuerpo -> va a cambiar cuando el master marque bien q serpiente
                    //queda en que lado
                    printf(") ");
                }
            }
            printf("\n");                      
        }
        for (unsigned int i = 0; i < game->num_players; i++){
            printf("player %d: x=%d y=%d;   is_blocked:%d, game_over:%d\n", i, game->players[i].x, game->players[i].y, game->players[i].is_blocked, game->game_over);
        }
        sem_post(&(sync->print_done));
        printf("\n");
    }

    
    return 0;
}
