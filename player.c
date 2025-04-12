// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "commonHeaders.h"
#include "utilities/sync.h"
#define TRUE 1



int main(int argc, char * argv[]){
    if(argc > 4) {
        perror("player recieved too many arguments");
        exit(EXIT_FAILURE);
    }
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t size = sizeof(GameState) + width * height * sizeof(int);

    GameState *game = open_existing_shm("/game_state", size, O_RDONLY);

    GameSync *sync = open_existing_shm("/game_sync", sizeof(GameSync), O_RDWR);

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
    bool first = TRUE;
    while(!game->game_over){
        //turnstile to avoid master starvation (otherwise multiple readers could access at the same time)
        sem_wait(&sync->master_utd);
        sem_post(&sync->master_utd);
        sem_wait(&sync->sig_var);
        
        sync->readers++;
        if (sync->readers == 1) {
            sem_wait(&sync->game_state_change);  // First reader locks game game
        }
        
        sem_post(&sync->sig_var);

        int game_finished = game->game_over || game->players[player_idx].is_blocked;        
        int ready = (moves != game->players[player_idx].v_moves + game->players[player_idx].inv_moves);
        
        // End of read
        sem_wait(&sync->sig_var);
        sync->readers--;
        if (sync->readers == 0) {
            sem_post(&sync->game_state_change);  // Last reader unlocks game game
        }
        sem_post(&sync->sig_var);

        if(game_finished) return 0;
        
        if(ready || first){
            unsigned char move = rand() % 8;
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