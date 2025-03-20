#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

#define SHM_STATE "/game_state"
#define SHM_SYNC  "/game_sync"

// Estructuras según la consigna
typedef struct {
    char name[16];
    unsigned int score;
    unsigned int invalid_moves;
    unsigned int valid_moves;
    unsigned short x, y;
    pid_t pid;
    bool can_move;
} Player;

typedef struct {
    unsigned short width, height;
    unsigned int num_players;
    Player players[9];
    bool game_over;
    int board[];
} GameState;

typedef struct {
    sem_t sem_A;
    sem_t sem_B;
    sem_t sem_C;
    sem_t sem_D;
    sem_t sem_E;
    unsigned int readers;
} GameSync;

// Función para verificar si un movimiento es válido
bool isValidMove(GameState *game, int player_index, unsigned char move) {
    int new_x = game->players[player_index].x;
    int new_y = game->players[player_index].y;

    switch (move) {
        case 0: new_y--; break; // Arriba
        case 1: new_x++; new_y--; break; // Diagonal superior derecha
        case 2: new_x++; break; // Derecha
        case 3: new_x++; new_y++; break; // Diagonal inferior derecha
        case 4: new_y++; break; // Abajo
        case 5: new_x--; new_y++; break; // Diagonal inferior izquierda
        case 6: new_x--; break; // Izquierda
        case 7: new_x--; new_y--; break; // Diagonal superior izquierda
        default: return false;
    }

    // Verificar límites del tablero
    if (new_x < 0 || new_x >= game->width || new_y < 0 || new_y >= game->height) {
        return false;
    }

    // Verificar si la celda está libre
    int cell_value = game->board[new_y * game->width + new_x];
    return cell_value > 0; // Celda libre si el valor es positivo
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    unsigned short width  = (unsigned short)atoi(argv[1]);
    unsigned short height = (unsigned short)atoi(argv[2]);

    // Conectarse a la memoria compartida del estado del juego
    int shm_fd = shm_open(SHM_STATE, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }

    // Leer el tamaño del estado del juego
    GameState temp_game;
    if (read(shm_fd, &temp_game, sizeof(GameState)) == -1) {
        perror("read game_state");
        exit(EXIT_FAILURE);
    }
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    // Mapear la memoria compartida del estado del juego
    GameState *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap game_state");
        exit(EXIT_FAILURE);
    }

    // Conectarse a la memoria compartida de sincronización
    int shm_sync_fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open game_sync");
        exit(EXIT_FAILURE);
    }

    // Mapear la memoria compartida de sincronización
    GameSync *sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap game_sync");
        exit(EXIT_FAILURE);
    }

    // Semilla para movimientos aleatorios
    srand(time(NULL));

    // Bucle principal del jugador
    while (!game->game_over) {
        // Esperar a que sea seguro leer el estado
        sem_wait(&sync->sem_C);
        sem_wait(&sync->sem_E);
        sync->readers++;
        if (sync->readers == 1) {
            sem_wait(&sync->sem_D);
        }
        sem_post(&sync->sem_E);
        sem_post(&sync->sem_C);

        // Leer el estado del juego
        bool still_running = !game->game_over;

        // Liberar el acceso al estado
        sem_wait(&sync->sem_E);
        sync->readers--;
        if (sync->readers == 0) {
            sem_post(&sync->sem_D);
        }
        sem_post(&sync->sem_E);

        if (!still_running) {
            break;
        }

        // Generar un movimiento aleatorio válido
        unsigned char move;
        do {
            move = rand() % 8;
        } while (!isValidMove(game, 0, move)); // 0 es el índice del jugador actual

        // Enviar el movimiento al máster
        if (write(STDOUT_FILENO, &move, sizeof(move)) == -1) {
            perror("write movimiento");
            exit(EXIT_FAILURE);
        }

        // Pequeña pausa para no saturar
        usleep(500000); // 0.5s
    }

    fprintf(stderr, "Jugador finaliza.\n");
    return 0;
}