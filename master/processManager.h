#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "../commonHeaders.h"
#include "master.h" 

void save_player_path(char * player_path, const int player_count);
void arg_handler(const int argc, char * const* argv);
void init_processes(GameState *game);

#endif