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
#include <time.h>
#include <sys/wait.h>
#include "master.h"
#define MAX_PLAYERS 9
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_WIDTH_STRING "10"
#define DEFAULT_HEIGHT_STRING "10"
#define DEFAULT_NANO_DELAY 200000
#define DEFAULT_TIMEOUT 10
#define HANDLE_OK 1
#define HANDLE_TIMEOUT 0
#define HANDLE_ERROR -1

// typedef struct ShmCDT * ShmADT;
// ShmADT create(const restrict char * id, size_t size);
// void destroy(ShmADT shm);
// ShmADT open(const restrict char * id);
// void close(ShmADT shm);
// ssize_t write(ShmADT shm, const void * buffer, size_t size);    //otro nombre de ADT o funcion?
// ssize_t write(ShmADT shm, void * buffer, size_t size);          //

char * width_string;
char * height_string;
int timeout = DEFAULT_TIMEOUT;
int delay = DEFAULT_NANO_DELAY;
char * view_path;
unsigned int seed;
char * player_paths[10]; //NULL TERMINATED
fd_set read_fds;
int highest_fd;
int current_player = 0;
int player_pipes[MAX_PLAYERS][2];

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
    printf("Saved player path[%d]: %s\n", player_count, player_paths[player_count]);
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

void init_processes(GameState *game, const char *view_path){
    // pid_t view_pid = 0;

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
            printf("About to execve: %s %s %s\n", player_path, width_string, height_string);
            fflush(stdout); // make sure it prints before execve

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


void init_board(GameState *game, unsigned int seed){
    srand(seed);

    size_t board_size = game->width * game->height;
    
    for(unsigned int i = 0; i < board_size; i++){
        game->board[i] = (rand() % 9) + 1;
    }
}

void init_game_state(GameState * game, int width, int height){
    game->game_over = false;
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
        game->players[game->num_players] = p;
        game->num_players++;
    }
}

int handle_moves_test(GameState * game, struct timeval * tv);
void process_player_move(GameState * game, int player_idx, unsigned char move);

void handle_moves(GameState * game, GameSync * sync);

int main (int const argc, char * const * argv){
    seed = time(NULL);
    width_string = DEFAULT_WIDTH_STRING;
    height_string = DEFAULT_HEIGHT_STRING;
    view_path = NULL;

    GameSync * sync = createSHM("/game_sync", sizeof(GameSync), 0666);

    arg_handler(argc, argv);

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int width = atoi(width_string);
    int height = atoi(height_string);
    
    printf("width: %d\nheight: %d\n", width, height);

    size_t size = sizeof(GameState) + width * height * sizeof(int);

    printf("Final size: %zu\n", size);
    
    GameState * game = createSHM("/game_state", size, 0644);
    init_game_state(game, width, height);

    init_board(game, seed);

    init_processes(game, view_path);

    sem_init(&sync->print_needed, 1, 0);
    sem_init(&sync->print_done, 1, 1); //  1, 0
    
    sem_init(&sync->game_state_change, 1, 1);
    sem_init(&sync->master_utd, 1, 1);
    sem_init(&sync->sig_var, 1, 1);
    sync->readers = 0;
    highest_fd = 0;

    // int last_cycle = time(NULL);
    while(!game->game_over){
        FD_ZERO(&read_fds);
        for(unsigned int i = 0; i < game->num_players; i++){
            FD_SET(player_pipes[i][0], &read_fds);
            if(player_pipes[i][0] > highest_fd) {
                highest_fd = player_pipes[i][0];
            }
        }
        highest_fd += 1; 

        sem_wait(&sync->master_utd);
        sem_wait(&sync->game_state_change);
        sem_post(&sync->master_utd);

        int resume = handle_moves_test(game, &tv);
        if(resume <= 0){
            game->game_over = true;
        }

        sem_post(&sync->game_state_change);
    }

    int wstatus;
    for(unsigned int i = 0; i < game->num_players; i++){
        int returned_pid = waitpid(game->players[i].pid, &wstatus, WEXITED);
        if(returned_pid == -1){
            printf("WAITPID failed on player (%s) with pid %d", game->players[i].name, game->players[i].pid);
        }
        if(WIFEXITED(wstatus)){
            printf("Player %s exited with status (%d)", game->players[i].name, wstatus);
        }
    }

    shm_unlink("/game_state");
    shm_unlink("/game_sync");



    return 0;
}

int handle_moves_test(GameState *game, struct timeval *timeout) {
    int ready = select(highest_fd, &read_fds, NULL, NULL, timeout);

    if (ready == -1) {
        perror("select error");
        return -1;
    } else if (ready == 0) {
        printf("Timeout reached. No input received.\n");
        return 0;
    }

    for (unsigned int i = 0; i < game->num_players; i++) {
        int fd = player_pipes[i][0];
        if (FD_ISSET(fd, &read_fds)) {
            unsigned char move;
            ssize_t bytes = read(fd, &move, sizeof(move));

            if (bytes <= 0) {
                perror("read from player");
                continue;
            }

            process_player_move(game, i, move);
        }
    }

    return 1;
}

void process_player_move(GameState * game, int player_idx, unsigned char move){
    int dirX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dirY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    printf("reached process_player_move");
    exit(0);
}

// int handle_moves(GameState * game, GameSync * sync, struct timeval * tv){

//     int player_moved = select(highest_fd, &read_fds, NULL, NULL, &tv);
//         if(player_moved == -1){
//             perror("select");
//             exit(EXIT_FAILURE);
//         }
//         if(player_moved == 0){
//             game->game_over = true; // fin
//             sem_post(&sync->print_needed); // view
//             sem_wait(&sync->print_done); // espero conf
//             printf("timeout");
//             sem_post(&sync->game_state_change); // free
//             return -1;
//         }

//         sem_wait(&sync->master_utd);
//         sem_wait(&sync->game_state_change);    

//         // int dirX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
//         // int dirY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

//         // for(int i = 0; i < game->num_players; i++){
//         //     int player_idx = (current_player + i) % game->num_players; // round robbin
//         //     if(FD_ISSET(player_pipes[player_idx][0], &read_fds)){
//         //         unsigned char direction;
//         //         int bytes = read(player_pipes[player_idx][0], &direction, sizeof(direction));
//         //         if(bytes <= 0){
//         //             game->players[player_idx].blocked = true;
//         //         } else {
//         //             int newPos_x = game->players[idx].x + dirX[direction];
//         //             int newPos_y = game->players[idx].y + dirY[direction];
//         //             int cell_idx = newPos_y * game->width + newPos_x;
//         //             bool valid_move = true;
//         //             if(newPos_x < 0 || newPos_x >= game->width || newPos_y < 0 || newPos_y >= game->height || direction > 7 || game->board[cell_idx] <= 0){
//         //                 game->players[player_idx].inv_moves++;
//         //                 valid_move = false;
//         //             }
//         //             if(valid_move){
//         //                 last_valid_time = time(NULL);
//         //                 game->players[player_idx].v_moves++;
//         //                 game->players[player_idx].score += game->board[cell_idx];
//         //                 game->players[player_idx].x = newPos_x;
//         //                 game->players[player_idx].y = newPos_y;
//         //                 game->board[cell_index] = -(player_idx+1);
//         //         }
//         //     }
//         //     current_player = (player_idx + 1) % game->num_players;
//         //     break; 
        
//         //     }
//         // }

        

        
// }



