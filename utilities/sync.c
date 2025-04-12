#include "sync.h"

void init_semaphores(GameSync * sync){
    sem_init(&sync->print_needed, 1, 0);
    sem_init(&sync->print_done, 1, 0);
    sem_init(&sync->master_utd, 1, 1);
    sem_init(&sync->game_state_change, 1, 1);
    sem_init(&sync->sig_var, 1, 1);
}

void destroy_shm(GameSync * sync, GameState * game, size_t game_size){
    sem_destroy(&sync->print_needed);
    sem_destroy(&sync->print_done);
    sem_destroy(&sync->master_utd);
    sem_destroy(&sync->game_state_change);
    sem_destroy(&sync->sig_var);

    munmap(sync, sizeof(GameSync));
    munmap(game, game_size);

    shm_unlink("/game_state");
    shm_unlink("/game_sync");
}

void close_shm(){
    shm_unlink("/game_state");
    shm_unlink("/game_sync");
}

void * create_shm(char * name, size_t size, mode_t mode){
    int fd = shm_open(name, O_RDWR | O_CREAT, mode); 
    if(fd == -1){                                
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if(-1 == ftruncate(fd, size)){
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void * p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if(p == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

void * open_existing_shm(char * name, size_t size, int permissions){
    int shm_fd = shm_open(name, permissions, 0);
    if  (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    int prot = PROT_READ;
    if (permissions == O_RDWR){
        prot |= PROT_WRITE;
    }

    GameSync * shm = (GameSync *) mmap(NULL, size, prot, MAP_SHARED, shm_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    return shm;
}