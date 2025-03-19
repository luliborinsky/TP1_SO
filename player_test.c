#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

typedef struct Player{
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int inv_moves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int v_moves; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short pos_x, pos_y; // Coordenadas x e y en el tablero
    pid_t player_pid; // Identificador de proceso
    bool can_play; // Indica si el jugador tiene movimientos válidos disponibles
} Player;    


int main(){
    int game_state_fd = shm_open("/game_state", O_RDWR, S_IRUSR | S_IWUSR);
    if (game_state_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    Player player1 = {"hola", 0, 0, 0, 0, 0,}

    return 0;
}