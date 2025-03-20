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
    bool can_play; // Indica si el jugador tiene movimientos válidos disponibles
} Player;    

typedef struct{
    unsigned short width, height; // dimensioens
    unsigned int num_players; // cantidad de jugadores
    Player players[9]; // lista de jugadores
    bool game_over; // si el juego termino o no
    int board[]; // tablero dinamico
}GameState;

int main() {
    int game_state_fd = shm_open("/game_state", O_RDWR, 0666);
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

    bool player_found;
    while (1) {
        printf("pos_x: %d   pos_y: %d\n", game->players[0].pos_x, game->players[0].pos_y);
     
        // Imprimir el tablero
        for (int i = 0; i < game->height; i++) {
            for (int j = 0; j < game->width; j++) {
                int cell = game->board[i * game->width + j];
                
                player_found = FALSE;
                for(int p = 0; p < game->num_players; p++){
                    if(i == game->players[p].pos_y && j == game->players[p].pos_x){
                        player_found = TRUE;
                        break;
                    }
                }

                if (cell < 0 || player_found) {
                    // La "cabeza" del jugador
                    printf("P ");
                } else if (cell > 0) {
                    // Recompensa
                    printf("%d ", cell); 
                } else {
                    // Celda vacía
                    printf(". ");
                }
            }
            printf("\n");
        }
        usleep(500000);
        printf("\n");
    }

    return 0;
}
