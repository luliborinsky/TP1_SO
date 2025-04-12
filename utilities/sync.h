#ifndef SYNC_H
#define SYNC_H

#include "../commonHeaders.h"

void init_semaphores(GameSync * sync);
void destroy_and_unlink_semaphores(GameSync * sync);
void * create_shm(char * name, size_t size, mode_t mode);
void * open_existing_shm(char * name, size_t size, int permissions);
#endif