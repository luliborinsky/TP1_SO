// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com


#include "master.h"

void save_player_path(char * player_path, const int player_count){
    player_paths[player_count] = player_path;
}

void arg_handler(const int argc, char * const* argv){
    int opt;
    opterr = 0;
    int player_count = 0;

    while((opt = getopt(argc, argv, "w:h:t:d:p:v:s:")) != -1){
        switch(opt){
            case 'w':
                width_string = optarg;
                break;
            case 'h':
                height_string = optarg;                
                break;

            case 't':
                timeout = atoi(optarg);
                if(timeout <= 0){
                    perror("timeout must be positive (seconds)");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'd':
                delay = atoi(optarg);
                if(delay < 0){
                    perror("delay must be positive");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'p':{
                save_player_path(optarg, player_count++);
                while(optind < argc && player_count < MAX_PLAYERS){
                    
                    if(argv[optind][0] == '-') break;

                    save_player_path(argv[optind++], player_count++);
                }
                if(player_count > 9){
                    perror("more than 9 players");
                    exit(EXIT_FAILURE);
                }
                player_paths[player_count] = NULL;
                break;
            }
                
            case 'v':
                view_path = optarg;
                break;

            case 's':
                seed = atoi(optarg);
                break;
                
            case '?':
                fprintf(stderr,
                    "ERROR: optopt = %d:\nUsage: %s [-w width] [-p player_paths (MAX 9)] [-h height] [-d delay (ms)] [-t timeout (s)] [-s seed] [-v view_path] \n",
                    optopt, argv[0]);
                exit(EXIT_FAILURE);

            default:
                fprintf(stderr,
                        "Usage: %s [-w width] [-p player_paths (MAX 9)] [-h height] [-d delay (ms)] [-t timeout (s)] [-s seed] [-v view_path] \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }   
    }
    if(player_count == 0){
        perror("ERROR: There are no players");
        exit(EXIT_FAILURE);
    }
}

void init_processes(GameState *game){
    if(view_path != NULL) {
        view_pid = fork();
        if(view_pid == -1){
            perror("View fork error");
            exit(EXIT_FAILURE);
        }
        if (view_pid == 0){
            if(access(view_path, X_OK) == -1){
                perror("View binary not accessible");
                exit(EXIT_FAILURE);
            }
            char * view_argv[4] = {view_path, width_string, height_string, NULL};
            char * envp[] = { NULL };

            execve(view_path, view_argv, envp);
            perror("execve failed"); // THIS LINE ONLY EXECUTES IF EXECVE FAILS
            exit(EXIT_FAILURE);
        }
    }
    //pipes for child comm
    for(unsigned int i = 0; i < game->num_players; i++){
        if(pipe(player_pipes[i]) == -1){
            perror("error creating pipe for player");
            exit(EXIT_FAILURE);
        }
    }
    
    // fork player
    for(unsigned int i = 0; i < game->num_players; i++){
        pid_t player_pid = fork();
        if(player_pid == -1){
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        if(player_pid == 0){
            close(player_pipes[i][0]); //close read-end
            if(dup2(player_pipes[i][1], STDOUT_FILENO) == -1){
                perror("dup failed");
                exit(EXIT_FAILURE);
            }
            close(player_pipes[i][1]);

            char *player_path = game->players[i].name; 

            if (access(player_path, X_OK) == -1) {
                perror("Player binary not accessible");
                exit(EXIT_FAILURE);
            }
            
            char *player_argv[4] = {player_path, width_string, height_string, NULL};
            char *envp[] = { NULL };

            execve(player_path, player_argv, envp);
            perror("execve failed"); // THIS LINE ONLY EXECUTES IF EXECVE FAILS
            exit(EXIT_FAILURE);
        }
        game->players[i].pid = player_pid;
    }
        
    for(unsigned int j = 0; j < game->num_players; j++){    
        close(player_pipes[j][1]);
    }
}


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
    
    printf("width: %d\nheight: %d\n", width, height);

    size_t size = sizeof(GameState) + width * height * sizeof(int);
    
    GameState * game = create_shm("/game_state", size, 0644);

    init_game_state(game, width, height);

    init_board(game, seed);

    init_processes(game);

    unsigned char player_moves[9];
    sync->readers = 0;
    //for first loop

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

    sem_post(&sync->print_needed);
    sem_wait(&sync->print_done);

    print_final_state(game, view_path, view_pid);

    destroy_shm(sync, game, size);

    return 0;
}
