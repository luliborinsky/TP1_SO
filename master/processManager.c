#include "processManager.h"


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