#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

// Nombres de la memoria compartida
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
    int board[]; // Tablero dinámico
} GameState;

typedef struct {
    sem_t sem_A; // Señala a la vista que hay cambios por imprimir
    sem_t sem_B; // Señala al máster que la vista terminó de imprimir
    sem_t sem_C; // Evita inanición del máster
    sem_t sem_D; // Mutex para el estado (escritor)
    sem_t sem_E; // Mutex para la variable readers
    unsigned int readers; // Cantidad de lectores
} GameSync;

int main(int argc, char* argv[]) {
    // 1) Recibir parámetros
    //    (Ancho y alto: no siempre lo usarás directamente, pero la consigna dice que debes recibirlos)
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    unsigned short width  = (unsigned short)atoi(argv[1]);
    unsigned short height = (unsigned short)atoi(argv[2]);

    // 2) Conectarse a la memoria compartida /game_state
    int shm_fd = shm_open(SHM_STATE, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }

    // Leer tamaño básico para mapear la estructura completa
    GameState temp_game;
    if (read(shm_fd, &temp_game, sizeof(GameState)) == -1) {
        perror("read game_state");
        exit(EXIT_FAILURE);
    }
    size_t game_size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);

    GameState* game = mmap(NULL, game_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap game_state");
        exit(EXIT_FAILURE);
    }

    // 3) Conectarse a la memoria compartida /game_sync
    int shm_sync_fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open game_sync");
        exit(EXIT_FAILURE);
    }

    GameSync* sync = mmap(NULL, sizeof(GameSync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap game_sync");
        exit(EXIT_FAILURE);
    }

    // Semilla para movimientos aleatorios
    srand(time(NULL));

    // 4) Bucle principal del jugador
    while (!game->game_over) {
        // ---- Lectores/Escritores: Ingresar como lector ----
        sem_wait(&sync->sem_C);          // Evitar inanición del máster
        sem_wait(&sync->sem_E);          // Bloquear la variable readers
        sync->readers++;
        if (sync->readers == 1) {
            // Si soy el primer lector, bloqueo sem_D (escritor)
            sem_wait(&sync->sem_D);
        }
        sem_post(&sync->sem_E);
        sem_post(&sync->sem_C);

        // ---- LEER el estado: ancho, alto, board, etc. ----
        // Por ejemplo, podrías revisar si el jugador puede moverse, 
        // buscar la ubicación, etc. Aquí simplemente tomamos info de `game`.
        bool still_running = !game->game_over; 
        // (Acá podrías hacer lógica más avanzada con `game->board`, `game->players`, etc.)

        // ---- Salir de la sección de lectura ----
        sem_wait(&sync->sem_E);
        sync->readers--;
        if (sync->readers == 0) {
            // Último lector libera al escritor
            sem_post(&sync->sem_D);
        }
        sem_post(&sync->sem_E);

        if (!still_running) {
            // Si el juego terminó mientras leíamos
            break;
        }

        // 5) Decidir un movimiento (aleatorio en este ejemplo)
        unsigned char move = rand() % 8;

        // 6) Enviar el movimiento al máster por STDOUT_FILENO
        if (write(STDOUT_FILENO, &move, sizeof(move)) == -1) {
            perror("write movimiento");
            exit(EXIT_FAILURE);
        }

        // Pequeña pausa para no saturar
        usleep(500000); // 0.5s
    }

    // Al terminar (si game->game_over == true), el jugador sale
    fprintf(stderr, "Jugador finaliza.\n");
    

    return 0;
}
