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
#include "processManager.h"

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

extern char * width_string;
extern char * height_string;
extern int timeout;
extern int delay;
extern char * view_path;
extern pid_t view_pid;
extern unsigned int seed;
extern char * player_paths[10]; //NULL TERMINATED
extern fd_set read_fds;
extern int highest_fd;
extern int current_player;
extern int player_pipes[MAX_PLAYERS][2];


#endif 