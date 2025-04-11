#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "../commonHeaders.h"

void init_board(GameState *game, unsigned int seed);
void init_game_state(GameState *game, int width, int height);
bool all_players_blocked(GameState *game);
int handle_moves(GameState *game, struct timeval *timeout, unsigned char *player_moves);
bool valid_move(GameState *game, int x, int y);
bool has_available_moves(GameState *game, int player_idx);
void process_player_move(GameState *game, int player_idx, unsigned char move);

#endif
