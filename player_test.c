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
    bool can_play; // Indica si el jugador tiene movimientos válidos disponibles
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
    sem_t C; // Mutex para evitar inanición del master al acceder al estado
    sem_t D; // Mutex para el estado del juego
    sem_t E; // Mutex para la siguiente variable
    unsigned int F; // Cantidad de jugadores leyendo el estado
} GameSync;
    

int main(){
    int game_state_fd = shm_open("/game_state", O_RDWR, 0666);
    if (game_state_fd == -1){
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }

    GameState temp_game;
    read(game_state_fd, &temp_game, sizeof(GameState));
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    GameState *game = mmap(NULL, game_size, PROT_READ | PROT_WRITE, MAP_SHARED, game_state_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap game_state");
        exit(EXIT_FAILURE);
    }

    int shm_sync_fd = shm_open("/game_sync", O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open sync");
        exit(EXIT_FAILURE);
    }

    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap sync");
        exit(EXIT_FAILURE);
    }

    pid_t my_pid = getpid();
    int player_index = -1;

    // Find the current player's index in the shared memory
    for (int i = 0; i < game->num_players; i++) {
        if (game->players[i].player_pid == my_pid) {
            player_index = i;
            break;
        }
    }

    if (player_index == -1) {
        fprintf(stderr, "Player not found in shared memory!\n");
        exit(EXIT_FAILURE);
    }

    unsigned char direction;
    int aux_x = game->players[player_index].pos_x;
    int aux_y = game->players[player_index].pos_y;

    srand(time(NULL));
    while(1){
        direction = rand() % 8;
        switch (direction) {
            case 0: aux_y--; break; // Up
            case 1: aux_y--; aux_x++; break; // Up-Right
            case 2: aux_x++; break; // Right
            case 3: aux_y++; aux_x++; break; // Down-Right
            case 4: aux_y++; break; // Down
            case 5: aux_y++; aux_x--; break; // Down-Left
            case 6: aux_x--; break; // Left
            case 7: aux_y--; aux_x--; break; // Up-Left
        }
        game->players[player_index].pos_x = aux_x;
        game->players[player_index].pos_y = aux_y;
        usleep(500000);
    }
    return 0;
}