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
#include <time.h>
#include <string.h>
#include "master.h"
#define TRUE 1



int main(){
    int game_state_fd = shm_open("/game_state", O_RDONLY, 0);
    if (game_state_fd == -1){
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }

    GameState temp_game;
    read(game_state_fd, &temp_game, sizeof(GameState));
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    GameState *game = (GameState *) mmap(NULL, game_size, PROT_READ, MAP_SHARED, game_state_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap game_state");
        exit(EXIT_FAILURE);
    }

    int shm_sync_fd = shm_open("/game_sync", O_RDWR, 0);
    if (shm_sync_fd == -1) {
        perror("shm_open sync");
        exit(EXIT_FAILURE);
    }

    GameSync *sync = (GameSync *) mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap sync");
        exit(EXIT_FAILURE);
    }

    int player_idx = 0;
    while(player_idx < 9 && getpid() != game->players[player_idx].pid){
        player_idx++;
    }
    if(player_idx >= 9){
        perror("player_idx not found");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    unsigned int moves = 0;
    int move = 0;
    bool first = TRUE;
    while(!game->game_over){
        //turnstile to avoid master starvation (otherwise multiple readers could access at the same time)
        sem_wait(&sync->master_utd);
        sem_post(&sync->master_utd);

        sem_wait(&sync->sig_var);
        
        sync->readers++;
        if (sync->readers == 1) {
            sem_wait(&sync->game_state_change);  // First reader locks game state
        }
        
        sem_post(&sync->sig_var);

        int game_finished = game->game_over || game->players[player_idx].is_blocked;        
        int ready = (moves != game->players[player_idx].v_moves + game->players[player_idx].inv_moves);
        
        // End of read
        sem_wait(&sync->sig_var);
        sync->readers--;
        if (sync->readers == 0) {
            sem_post(&sync->game_state_change);  // Last reader unlocks game state
        }
        sem_post(&sync->sig_var);

        if(game_finished) return 0;
        
        if(ready || first){
            move = rand() % 8;
            if (write(STDOUT_FILENO, &move, sizeof(move)) == -1) {
                perror("write movimiento");
                exit(EXIT_FAILURE);
            }
            first = !first;
            moves = game->players[player_idx].v_moves + game->players[player_idx].inv_moves;
        }
    }
    
    return 0;
}