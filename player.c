#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#define TRUE 1

typedef struct Player{
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int inv_moves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int v_moves; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short pos_x, pos_y; // Coordenadas x e y en el tablero
    pid_t player_pid; // Identificador de proceso
    bool is_blocked; // Indica si el jugador tiene movimientos bloqueados disponibles
} Player;    

typedef struct{
    unsigned short width, height; // dimensioens
    unsigned int num_players; // cantidad de jugadores
    Player players[9]; // lista de jugadores
    bool game_over; // si el juego termino o no
    int board[]; // tablero dinamico
}GameState;

typedef struct {
    sem_t A; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t B; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t master_utd; // Mutex para evitar inanición del master al acceder al estado
    sem_t game_state_change; // Mutex para el estado del juego
    sem_t sig_var; // Mutex para la siguiente variable
    unsigned int readers; // Cantidad de jugadores leyendo el estado
} GameSync;
    

int main(){
    int game_state_fd = shm_open("/game_state", O_RDONLY, 0);
    if (game_state_fd == -1){
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }

    GameState temp_game;
    read(game_state_fd, &temp_game, sizeof(GameState));
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    GameState *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, game_state_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap game_state");
        exit(EXIT_FAILURE);
    }

    int shm_sync_fd = shm_open("/game_sync", O_RDWR, 0);
    if (shm_sync_fd == -1) {
        perror("shm_open sync");
        exit(EXIT_FAILURE);
    }

    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap sync");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    int move;
    while(!game->game_over){
        sem_wait(&sync->master_utd);
        sem_wait(&sync->sig_var);
        sync->readers++;
        if (sync->readers == 1) {
            sem_wait(&sync->game_state_change);
        }
        sem_post(&sync->sig_var);
        sem_post(&sync->master_utd);

        // read game_state
        bool still_running = !game->game_over; 

        // release game_state
        sem_wait(&sync->sig_var);
        sync->readers--;
        if (sync->readers == 0) {
            sem_post(&sync->game_state_change);
        }
        sem_post(&sync->sig_var);

        // If the game is over, release access before exiting
        if (!still_running) break;

        move = rand() % 8;
        if (write(STDOUT_FILENO, &move, sizeof(move)) == -1) {
            perror("write movimiento");
            exit(EXIT_FAILURE);
        }

        //this sleep should be synched to the game delay
        //if absent the pipe between player and master fills up and causes a deadlock when the game finishes
        //master ends and waits for player but player is waiting to be read (i think)
        usleep(200000);
    }
    
    return 0;
}