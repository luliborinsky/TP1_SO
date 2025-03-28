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
#define TRUE 1
#define FALSE 0

typedef struct Player{
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int inv_moves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int v_moves; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short pos_x, pos_y; // Coordenadas x e y en el tablero
    pid_t player_pid; // Identificador de proceso
    bool is_blocked; // Indica si el jugador tiene movimientos bloqueados
} Player;    

typedef struct{
    unsigned short width, height; // dimensioens
    unsigned int num_players; // cantidad de jugadores
    Player players[9]; // lista de jugadores
    bool game_over; // si el juego termino o no
    int board[]; // tablero dinamico
}GameState;

typedef struct {
    sem_t print_needed; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t print_done; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t C; // Mutex para evitar inanición del master al acceder al estado
    sem_t D; // Mutex para el estado del juego
    sem_t E; // Mutex para la siguiente variable
    unsigned int F; // Cantidad de jugadores leyendo el estado
} GameSync;

int main() {
    int game_state_fd = shm_open("/game_state", O_RDONLY, 0644);
    if(game_state_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Leer valores básicos para saber el tamaño real:
    GameState temp_game;
    read(game_state_fd, &temp_game, sizeof(GameState));
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    // Mapear el estado real:
    GameState *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, game_state_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap");
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

    while (!game->game_over) {
        sem_wait(&(sync->print_needed));
        
        // Imprimir el tablero
        for (int i = 0; i < game->height; i++) {
            for (int j = 0; j < game->width; j++) {
                int cell = game->board[i * game->width + j];
                if (cell > 0) {
                    // Recompensa
                    printf("%d ", cell); 
                } else {
                    // Cabeza y cuerpo -> va a cambiar cuando el master marque bien q serpiente
                    //queda en que lado
                    printf("P ");
                }
            }
            printf("\n");                      
        }
        for (unsigned int i = 0; i < game->num_players; i++){
            printf("player %d: x=%d y=%d;   is_blocked:%d, game_over:%d\n", i, game->players[i].pos_x, game->players[i].pos_y, game->players[i].is_blocked, game->game_over);
        }
        sem_post(&(sync->print_done));
        printf("\n");
    }

    
    return 0;
}
