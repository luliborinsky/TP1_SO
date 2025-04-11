// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <math.h>
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
#include "commonHeaders.h"
#define MAX_PLAYERS 9
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_WIDTH_STRING "10"
#define DEFAULT_HEIGHT_STRING "10"
#define DEFAULT_MS_DELAY 200
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
int delay = DEFAULT_MS_DELAY;
char * view_path;
pid_t view_pid;
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
            char * view_argv[2] = {view_path, NULL};
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


void init_board(GameState *game, unsigned int seed){
    srand(seed);

    size_t board_size = game->width * game->height;
    
    for(unsigned int i = 0; i < board_size; i++){
        game->board[i] = (rand() % 9) + 1;
    }

    int zones_per_row = (int)ceil(sqrt(game->num_players));
    int zone_width = game->width / zones_per_row;
    int zone_height = game->height / zones_per_row;

    for (unsigned int i = 0; i < game->num_players; i++) {
        int zone_col = i % zones_per_row;
        int zone_row = i / zones_per_row;

        int x_center = zone_col * zone_width + zone_width / 2;
        int y_center = zone_row * zone_height + zone_height / 2;

        game->players[i].x = x_center;
        game->players[i].y = y_center;
        game->board[y_center * game->width + x_center] = 0;
        printf("player %d on x=%d y=%d\n", i, x_center, y_center);
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
    while(player_paths[game->num_players] != NULL && game->num_players < 10){
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

int handle_moves(GameState * game, struct timeval * tv, unsigned char * player_moves);
void process_player_move(GameState * game, int player_idx, unsigned char move);
bool has_available_moves(GameState * game, int player_idx);


bool all_players_blocked(GameState * game){
    for(unsigned int i = 0; i < game->num_players; i++){
        if(!game->players[i].is_blocked) return false;
    }
    return true;
}

int main (int const argc, char * const * argv){
    seed = time(NULL);
    width_string = DEFAULT_WIDTH_STRING;
    height_string = DEFAULT_HEIGHT_STRING;
    view_path = NULL;

    GameSync * sync = createSHM("/game_sync", sizeof(GameSync), 0666);
    sem_init(&sync->print_needed, 1, 0);
    sem_init(&sync->print_done, 1, 0);
    sem_init(&sync->game_state_change, 1, 1);
    sem_init(&sync->master_utd, 1, 1);
    sem_init(&sync->sig_var, 1, 1);

    arg_handler(argc, argv);

    if(view_path == NULL) delay = 0;

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
            sem_post(&sync->print_needed);
            game->game_over = true;
        } 

        
        usleep(delay * 1000);
    }
        
    
    printf("game is %s over\n", game->game_over? "" : "NOT");

    for(unsigned int i = 0; i<game->num_players; i++){
        printf("player is %s blocked\n", game->players[i].is_blocked ? "" : "NOT");
    }

    int wstatus;
    int returned_pid;
    for(unsigned int i = 0; i < game->num_players; i++){
        returned_pid = waitpid(game->players[i].pid, &wstatus, 0);
        if(returned_pid == -1){
            printf("WAITPID failed on player (%s) with pid %d\n", game->players[i].name, game->players[i].pid);
        }
        if(WIFEXITED(wstatus)){
            printf("Player %s exited with status (%d)\n", game->players[i].name, wstatus);
        }
    }

    if(view_path != NULL){
        returned_pid = waitpid(view_pid, &wstatus, 0);
        if(returned_pid == -1){
            printf("WAITPID failed on view with pid %d\n", view_pid);
        }
        if(WIFEXITED(wstatus)){
            printf("View exited with status (%d)\n", wstatus);
        }
    }


    sem_destroy(&sync->print_needed);
    sem_destroy(&sync->print_done);
    sem_destroy(&sync->master_utd);
    sem_destroy(&sync->game_state_change);
    sem_destroy(&sync->sig_var);

    shm_unlink("/game_state");
    shm_unlink("/game_sync");

    return 0;
}

//returns != 0 if failed
int handle_moves(GameState *game, struct timeval *timeout, unsigned char * player_moves) {
    int ready = select(highest_fd, &read_fds, NULL, NULL, timeout);

    if (ready == -1) {
        perror("select error\n");
        return -1;
    } else if (ready == 0) {
        printf("Timeout reached. No input received.\n");
        return -1;
    }

    for (unsigned int i = 0; i < game->num_players; i++) {
        int fd = player_pipes[i][0];
        if (FD_ISSET(fd, &read_fds)) {
            unsigned char move;

            if (read(fd, &move, sizeof(move)) <= 0) {
                perror("read from player\n");
                continue;
            }

            player_moves[i] = move;
        }
    }

    return 0;
}

bool valid_move(GameState * game, int x, int y){
    if(x < 0 || x >= game->width || y < 0 || y >= game->height){
        return false;
    }
    if(game->board[y * game->width + x] > 0) {
        return true;
    }
    return false;
}

bool has_available_moves(GameState * game, int player_idx){
    int dir_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dir_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    for (int i = 0; i < 8; i++){
        int new_x = game->players[player_idx].x + dir_x[i];
        int new_y = game->players[player_idx].y + dir_y[i];

        if(valid_move(game, new_x, new_y)) return true;
    }

    return false;
}

void process_player_move(GameState * game, int player_idx, unsigned char move){
    int dir_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dir_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    int new_x = game->players[player_idx].x + dir_x[move];
    int new_y = game->players[player_idx].y + dir_y[move];

    if(!has_available_moves(game, player_idx)){
        game->players[player_idx].is_blocked = true;
        return;
    }

    if(!valid_move(game, new_x, new_y)){
        game->players[player_idx].inv_moves++;
        return;
    }

    game->players[player_idx].v_moves++;
    game->players[player_idx].x = new_x;
    game->players[player_idx].y = new_y;
    game->players[player_idx].score += game->board[new_y + game->width + new_x];
    game->board[new_y * game->width + new_x] = 0;
}