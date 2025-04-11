#include "sync.h"

void init_semaphores(GameSync * sync){
    sem_init(&sync->print_needed, 1, 0);
    sem_init(&sync->print_done, 1, 0);
    sem_init(&sync->master_utd, 1, 1);
    sem_init(&sync->game_state_change, 1, 1);
    sem_init(&sync->sig_var, 1, 1);
}

void destroy_and_unlink_semaphores(GameSync * sync){
    sem_destroy(&sync->print_needed);
    sem_destroy(&sync->print_done);
    sem_destroy(&sync->master_utd);
    sem_destroy(&sync->game_state_change);
    sem_destroy(&sync->sig_var);

    shm_unlink("/game_state");
    shm_unlink("/game_sync");
}