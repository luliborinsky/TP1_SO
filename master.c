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
#include <string.h>
#include "master.h"
#define MAX_PLAYERS 9
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_NANO_DELAY 200000
#define DEFAULT_TIMEOUT 10

// typedef struct ShmCDT * ShmADT;
// ShmADT create(const restrict char * id, size_t size);
// void destroy(ShmADT shm);
// ShmADT open(const restrict char * id);
// void close(ShmADT shm);
// ssize_t write(ShmADT shm, const void * buffer, size_t size);    //otro nombre de ADT o funcion?
// ssize_t write(ShmADT shm, void * buffer, size_t size);          // ^ 


int timeout = DEFAULT_TIMEOUT;
int delay = DEFAULT_NANO_DELAY;
char * view_path = NULL;
unsigned int seed = 0;

//creates shared memory
void * createSHM(char * name, size_t size, mode_t mode){
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, mode); 
    if(fd == -1){                                
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if(strcmp(name, "/game_state") == 0){
        GameState temp_game;
        read(fd, &temp_game, sizeof(GameState));
        size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);
    }

    if(-1 == ftruncate(fd, size)){
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void * p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if(p == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

void process_player(GameState * game, char * player_name){    
    Player * p = &game->players[game->num_players];
    strncpy(p->name, player_name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->score = 0;
    p->inv_moves = 0;
    p->v_moves = 0;
    p->is_blocked = false;
    game->num_players++;
    //positions and pid are initialized at game start
}

void arg_handler(int argc, char ** argv, GameState * game){
    int opt;
    opterr = 0;
    memset(game, 0, sizeof(GameState));

    while((opt = getopt(argc, argv, "w:h:t:d:p:v:s:")) != -1){
        switch(opt){
            case 'w':
                game->width = atoi(optarg);
                if(game->width < DEFAULT_WIDTH){
                    perror("board size too small");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                game->height = atoi(optarg);
                if(game->height < DEFAULT_HEIGHT){
                    perror("board size too small");
                    exit(EXIT_FAILURE);
                }
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
                process_player(game, optarg);
                while(optind < argc && game->num_players < MAX_PLAYERS){
                    
                    if(argv[optind][0] == '-') break;

                    process_player(game, argv[optind++]);
                }
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
    if(game->num_players == 0){
        perror("must specify number of players");
        exit(EXIT_FAILURE);
    }
}

void init_processes(GameState *game, const char *view_path, int player_pipes[][2]){
    // pid_t view_pid = 0;

    // pipe para players (NOSE)
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
            for(unsigned int j = 0; j <= i; j++){
                close(player_pipes[j][0]);
                close(player_pipes[j][1]);
            }
            exit(EXIT_FAILURE);
        }
        if(player_pid == 0){ // hijo
            for(unsigned int j = 0; j < game->num_players; j++){
                if(j != i){
                    close(player_pipes[j][0]);
                    close(player_pipes[j][1]);
                }
            }
            close(player_pipes[i][0]);  // close read

            if(dup2(player_pipes[i][1], STDOUT_FILENO) == -1){
                perror("error stdout player");
                exit(EXIT_FAILURE);
            }
            close(player_pipes[i][1]);
            char * player_argv[1];
            player_argv[0] = game->players[i].name;
            execve(game->players[i].name, player_argv, player_argv);
        }
        else { // padre (master)
            close(player_pipes[i][1]); // close write
            game->players[i].pid = player_pid;
            printf("player %d (%s) init with pid: %d\n", i + 1, game->players[i].name, player_pid); 
        }

        

    }



}

void init_board(GameState *game, unsigned int seed){
    srand(seed);

    size_t board_size = game->width * game->height;
    
    // lleno tablero recompensas
    for(unsigned int i = 0; i < board_size; i++){
        game->board[i] = (rand() % 9) + 1;
    }
}

int main (int argc, char ** argv){    
    //create SHMs
    GameState * game = createSHM("/game_state", sizeof(GameState), 0644); //size of GameState is defined on createSHM
    GameSync * sync = createSHM("/game_sync", sizeof(GameSync), 0666);
    
    game->num_players = 0;
    game->width = DEFAULT_WIDTH;
    game->height = DEFAULT_HEIGHT;

    arg_handler(argc, argv, game);
    
    if(game->num_players == 0){
        perror("ERROR: no players");
        exit(EXIT_FAILURE);
    }
    
    init_board(game, seed); //TODO

    int player_pipes[game->num_players][2];

    init_processes(game, view_path, player_pipes); //TODO
    return 0;
}



