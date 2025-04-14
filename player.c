// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "commonHeaders.h"
#include "utilities/sync.h"
#define TRUE 1

int dir_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
int dir_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
char last_center = 'y';

int to_center(GameState * game, int best, int best_2, int x, int y){
    int center_x = game->width / 2;
    int center_y = game->height / 2;

    int new_x = x + dir_x[best];
    int new_y = y + dir_y[best];
    int new_x2 = x + dir_x[best_2];
    int new_y2 = y + dir_y[best_2];
    
    int p1x = new_x - center_x;
    int p1y = new_y - center_y;
    int p2x = new_x2 - center_x;
    int p2y = new_y2 - center_y;

    p1x = p1x > 0 ? p1x : -p1x;
    p1y = p1y > 0 ? p1y : -p1y;
    p2x = p2x > 0 ? p2x : -p2x;
    p2y = p2y > 0 ? p2y : -p2y;

    if(p1x < p2x){
        if(p1y < p2y || last_center == 'y'){
            last_center = 'x';
            return best_2;
        }
    } 

    last_center = 'y';
    return best;
}

int determine_move(GameState * game, int player_idx){
    int x = game->players[player_idx].x;
    int y = game->players[player_idx].y;

    int points = 0, highest_points = 0;
    int best_move = -1;
    for(int i = 0; i < 8; i++){
        int new_x = x + dir_x[i];
        int new_y = y + dir_y[i];
        if(!valid_move(game, new_x, new_y)){
            continue;
        }

        points = game->board[new_y * game->width + new_x];

        if (points > highest_points) {
            highest_points = points;
            best_move = i;
        }
        else if (points == highest_points){
            best_move = to_center(game, best_move, i, x, y);
        }
    }

    if(best_move == -1){
        perror("couldnt determine move");
        exit(EXIT_FAILURE);
    }
    return best_move;
}


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
    while(!game->game_over){
        
        sem_wait(&sync->turnstile);
        sem_post(&sync->turnstile);
        sem_wait(&sync->readers_critical_section);
        
        sync->readers++;
        if (sync->readers == 1) {
            sem_wait(&sync->game_state_change);  
        }
        sem_post(&sync->readers_critical_section);

        int game_finished = game->game_over || game->players[player_idx].is_blocked;        
        int ready = (moves == game->players[player_idx].v_moves + game->players[player_idx].inv_moves);
        

        // End of read
        sem_wait(&sync->readers_critical_section);
        sync->readers--;
        if (sync->readers == 0) {
            sem_post(&sync->game_state_change);  
        }
        sem_post(&sync->readers_critical_section);

        if(game_finished) return 0;
        

        if(ready){
            unsigned char move = determine_move(game, player_idx);
            if (write(STDOUT_FILENO, &move, sizeof(move)) == -1) {
                perror("write movimiento");
                exit(EXIT_FAILURE);
            }
            moves++;
        }
    }

    close_shm();
    return 0;
}