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

void save_player_path(char *player_path, const int player_count);
void arg_handler(const int argc, char *const *argv);
void init_processes(GameState *game);

#endif 