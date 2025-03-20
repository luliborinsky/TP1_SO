#ifndef BOARD_H
#define BOARD_H

#include <sys/types.h>   
#include <stdbool.h>


typedef struct {
    char name[16];
    unsigned int score;
    unsigned int invalid_moves;
    unsigned int valid_moves;
    unsigned short x, y;
    pid_t pid;
    bool can_move;
} Player;

typedef struct {
    unsigned short width, height;
    unsigned int num_players;
    Player players[9];
    bool game_over;
    int board[]; 
} GameState;


void printBoard(GameState *game);

#endif 
