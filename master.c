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

char * width_string;
char * height_string;
int timeout = DEFAULT_TIMEOUT;
int delay = DEFAULT_NANO_DELAY;
char * view_path = NULL;
unsigned int seed = 0;
char * player_paths[10]; //NULL TERMINATED


void * createSHM(char * name, size_t size, mode_t mode){
    int fd = shm_open(name, O_RDWR | O_CREAT, mode); 
    if(fd == -1){                                
        perror("shm_open");
        exit(EXIT_FAILURE);
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
                player_paths[player_count] = "\0";
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

void init_processes(GameState *game, const char *view_path){
    // pid_t view_pid = 0;

    //pipes for child comm
    int player_pipes[game->num_players][2];
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
            int prev_write_fd = player_pipes[i][1];
            if(dup2(prev_write_fd, STDIN_FILENO) == -1){
                perror("dup failed");
                exit(EXIT_FAILURE);
            }
            close(prev_write_fd);
            close(player_pipes[i][0]); //close read-end

            char * player_path = game->players[i].name;
            char * player_argv[4] = {player_path, width_string, height_string, NULL}; //ChompChamps lo ahce asi
            char * envp[] = {'\0'};

            game->players[i].pid = getpid();
            execve(player_path, player_argv, envp);
        }
    }
        
    for(unsigned int j = 0; j < game->num_players; j++){    
        close(player_pipes[j][1]);
    }
}
//strace // 265 was master pid and 270 child pid
// 265   pipe2([5, 6], 0)                  = 0
// 265   clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f0dc74f2a10) = 270
// 270   set_robust_list(0x7f0dc74f2a20, 24 <unfinished ...>
// 265   close(6 <unfinished ...>
// 270   <... set_robust_list resumed>)    = 0
// 265   <... close resumed>)              = 0
// 265   pselect6(6, [5], NULL, NULL, {tv_sec=10, tv_nsec=0}, NULL <unfinished ...>
// 270   setuid(1000)                      = 0
// 270   close(5)                          = 0
// 270   dup2(6, 1)                        = 1
// 270   close(6)                          = 0
// 270   execve("bin/player", ["bin/player", "10", "10"], 0x7fff859a5e18 /* 0 vars */) = 0


void init_board(GameState *game, unsigned int seed){
    srand(seed);

    size_t board_size = game->width * game->height;
    
    for(unsigned int i = 0; i < board_size; i++){
        game->board[i] = (rand() % 9) + 1;
    }
}

void init_game_state(GameState * game, int width, int height){
    game->game_over = true;
    game->num_players = 0;
    game->width = width;
    game->height = height;
    if(game->height < DEFAULT_HEIGHT){
        perror("height must be higher than 9");
        exit(EXIT_FAILURE);
    }
    if(game->width < DEFAULT_WIDTH){
        perror("width must be higher than 9");
        exit(EXIT_FAILURE);
    }

    Player p;
    while(player_paths[game->num_players] != NULL && game->num_players < 9){
        strncpy(p.name, player_paths[game->num_players], sizeof(p.name) - 1);

        p.name[sizeof(p.name) - 1] = '\0';
        p.score = 0;
        p.inv_moves = 0;
        p.v_moves = 0;
        p.is_blocked = false;
        game->players[game->num_players++] = p;
    }
}

int main (int const argc, char * const * argv){
    width_string = "10";
    height_string = "10";

    GameSync * sync = createSHM("/game_sync", sizeof(GameSync), 0666);

    arg_handler(argc, argv);

    int width = atoi(width_string);
    int height = atoi(height_string);
    
    printf("width: %d\nheight: %d\n", width, height);

    size_t size = sizeof(GameState) + width * height * sizeof(int);

    printf("Final size: %zu\n", size);
    
    GameState * game = createSHM("/game_state", size, 0644);
    init_game_state(game, width, height);

    init_board(game, seed);

    init_processes(game, view_path);
    return 0;
}



