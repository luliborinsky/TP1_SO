#ifndef SYNC_H
#define SYNC_H

#include "../commonHeaders.h"

void init_semaphores(GameSync * sync);
void destroy_and_unlink_semaphores(GameSync * sync);

#endif