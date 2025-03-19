#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"


typedef struct{
    char name[16]; // nombre del jugador
    unsigned int score; // puntaje
    unsigned int invalid_moves; // movimientos invalidos
    unsigned int valid_moves; // movimientos validos
    unsigned short x,y; // cordenada en el tablero
    pid_t pid; // identificador del proceso
    bool can_move; // 
}Player;
    
typedef struct{
    unsigned short width, height; // dimensioens
    unsigned int num_players; // cantidad de jugadores
    Player players[9]; // lista de jugadores
    bool game_over; // si el juego termino o no
    int board[]; // tablero dinamico
}GameState;

// semaforps
typedef struct{
    sem_t sem_A; // indica  a la vista que hay cambios por imprimir
    sem_t sem_B; // indica al master que la vista termino de imprimir
    sem_t sem_C; // mutex para evitar la inanicion del master
    sem_t sem_D; // mutex para el estado del juego
    sem_t sem_E; // mutex para la variable de lectores
    unsigned int readers; // cantidad de lectores
}GameSync;

int main(){
    int shm_fd = shm_open(SHM_STATE, O_RDWR,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(shm_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    GameState temp_game;
    read(shm_fd, &temp_game, sizeof(GameState));
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    GameState *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    int shm_sync_fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open sync");
        exit(EXIT_FAILURE);
    }

    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap sync");
        exit(EXIT_FAILURE);
    }
    

    while (1) {
        sem_wait(&sync->sem_A);  // Esperar señal del máster
        system("clear");
        
        // Imprimir el tablero
        for (int i = 0; i < game->height; i++) {
            for (int j = 0; j < game->width; j++) {
                int cell = game->board[i * game->width + j];

                bool is_player = false;
                for(unsigned int p = 0; p < game->num_players; p++){
                    if(game->players[p].x == j && game->players[p].y == i){
                        is_player = true;
                        break;
                    }
                }

                if(!is_player){
                    if (cell > 0) {
                        printf("P ");  // Recompensa
                    } else if (cell < 0) {
                        printf("%d", cell);  // Celda ocupada por un jugador
                    } else {
                        printf(". ");  // Celda vacía
                    }
                }

                // if (cell > 0) {
                //     printf("%d ", cell);  // Recompensa
                // } else if (cell < 0) {
                //     printf("P%d", -cell);  // Celda ocupada por un jugador
                // } else {
                //     printf(". ");  // Celda vacía
                // }
            }
            printf("\n");
        }
        printf("\n");

        // Imprimir información de los jugadores
        for (unsigned int i = 0; i < game->num_players; i++) {
            printf("%s: %d puntos (%d, %d)\n", game->players[i].name, game->players[i].score, game->players[i].x, game->players[i].y);
        }
        printf("\n");

        sem_post(&sync->sem_B);  // Avisar al máster que terminó de imprimir
        sleep(1);
    }
    
    return 0;
}





