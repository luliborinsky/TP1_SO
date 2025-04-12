#ifndef MASTER_H
#define MASTER_H

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
#include "../commonHeaders.h"
#include "../utilities/sync.h"
#include "gameLogic.h"

void *createSHM(char *name, size_t size, mode_t mode);
void save_player_path(char *player_path, const int player_count);
void arg_handler(const int argc, char *const *argv);
void init_processes(GameState *game);
void init_board(GameState *game, unsigned int seed);
void init_game_state(GameState *game, int width, int height);
int handle_moves(GameState *game, struct timeval *timeout, unsigned char *player_moves);
void process_player_move(GameState *game, int player_idx, unsigned char move);
bool has_available_moves(GameState *game, int player_idx);
bool valid_move(GameState *game, int x, int y);
bool all_players_blocked(GameState *game);

#endif 