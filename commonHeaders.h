#ifndef COMMON_H
#define COMMON_H

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
#include <math.h>

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

extern char * player_paths[10];
extern fd_set read_fds;
extern int highest_fd;
extern int player_pipes[MAX_PLAYERS][2];

typedef struct Player{
    char name[16]; 
    unsigned int score;
    unsigned int inv_moves; 
    unsigned int v_moves; 
    unsigned short x, y; 
    pid_t pid; 
    bool is_blocked; 
} Player;

typedef struct{
    unsigned short width, height; 
    unsigned int num_players; 
    Player players[9]; 
    bool game_over; 
    int board[]; 
}GameState;

typedef struct {
    sem_t print_needed; 
    sem_t print_done; 
    sem_t turnstile;
    sem_t game_state_change; 
    sem_t readers_critical_section; 
    unsigned int readers; 
} GameSync;



#endif 