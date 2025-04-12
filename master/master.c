// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "master.h"


char *width_string = NULL;
char *height_string = NULL;
int timeout = DEFAULT_TIMEOUT;
int delay = DEFAULT_MS_DELAY;
char *view_path = NULL;
pid_t view_pid = -1;
unsigned int seed = 0;
char *player_paths[10] = {NULL}; // Initialize to NULL
fd_set read_fds;
int highest_fd = 0;
int current_player = 0;
int player_pipes[MAX_PLAYERS][2];

int main (int const argc, char * const * argv){
    seed = time(NULL);
    width_string = DEFAULT_WIDTH_STRING;
    height_string = DEFAULT_HEIGHT_STRING;
    view_path = NULL;

    GameSync * sync = create_shm("/game_sync", sizeof(GameSync), 0666);
    init_semaphores(sync);

    arg_handler(argc, argv);

    if(view_path == NULL) delay = 0;

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int width = atoi(width_string);
    int height = atoi(height_string);

    size_t size = sizeof(GameState) + width * height * sizeof(int);
    
    GameState * game = create_shm("/game_state", size, 0644);

    init_game_state(game, width, height);

    init_board(game, seed);

    init_processes(game);

    unsigned char player_moves[9];
    sync->readers = 0;
    //for first loop

    initial_print(game, width, height, delay, timeout, seed, view_path);

    if(view_path != NULL){
            sem_post(&sync->print_needed);
            sem_wait(&sync->print_done);
    }
    sem_post(&sync->game_state_change);
    
    while(!game->game_over){
        highest_fd = 0;
        FD_ZERO(&read_fds);
        for(unsigned int i = 0; i < game->num_players; i++){
            FD_SET(player_pipes[i][0], &read_fds);
            if(player_pipes[i][0] >= highest_fd) {
                highest_fd = player_pipes[i][0] + 1;
            }
        }

        if(handle_moves(game, &tv, player_moves)){
            printf("handle_moves error\n");
            game->game_over = true;
        }
        
        sem_wait(&sync->master_utd);
        sem_wait(&sync->game_state_change);
        sem_post(&sync->master_utd);

        for (unsigned int i = 0; i<game->num_players; i++){
            process_player_move(game, i, player_moves[i]);
        }

        sem_post(&sync->game_state_change);

        if(view_path != NULL){
            sem_post(&sync->print_needed);
            sem_wait(&sync->print_done);
        }

        if(all_players_blocked(game)){
            printf("all players blocked\n");
            game->game_over = true;
        } 

        
        usleep(delay * 1000);
    }   

    if(view_path != NULL){
        sem_post(&sync->print_needed);
        sem_wait(&sync->print_done);
    }

    print_final_state(game, view_path, view_pid);

    destroy_shm(sync, game, size);

    return 0;
}
