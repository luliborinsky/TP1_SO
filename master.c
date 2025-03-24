#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#define TRUE 1
#define MAX_ARG_COUNT 23
#define MIN_BOARD_SIZE 10
#define MAX_PLAYERS 9

typedef enum{
    ARG_WIDTH, // -w 
    ARG_HEIGHT, // -h
    ARG_TIMEOUT, // -t
    ARG_DELAY, // -d
    ARG_VIEW, // -v
    ARG_PLAYERS // -p
} ArgType;

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
    sem_t print_needed; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t print_done; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t master_utd; // Mutex para evitar inanición del master al acceder al estado
    sem_t game_state_change; // Mutex para el estado del juego
    sem_t sig_var; // Mutex para la siguiente variable
    unsigned int readers; // Cantidad de jugadores leyendo el estado
} GameSync;


//creates shared memory
void * createSHM(char * name, size_t size, mode_t mode){
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, mode); 
    if(fd == -1){                                
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if(strcmp(name, "/game_state") == 0){
        GameState temp_game;
        read(fd, &temp_game, sizeof(GameState));
        size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);
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

int process_players(char ** argv, int argc, int idx, GameState * game_state){
    int i = idx;
    if(argv[i][0] == '-' || argv[i][0] == ' ' || argv[i][0] == '\n'){
        perror("bad usage of -p [players]");
        exit(EXIT_FAILURE);
    }

    while(argv[i][0] !=  '-' && i < argc){
        //initialize players
        printf("initializing players");
        i++;
    }

    // Player *p = &game_state->players[game_state->num_players];
    
    //     p->score = 0;
    //     p->inv_moves = 0;
    //     p->v_moves = 0;
    //     p->can_play = true;
       
    //     p->pos_x = rand() % game_state->width;
    //     p->pos_y = rand() % game_state->height;

    //     printf("Jugador %d inicializado: %s (posición: %d,%d)\n", 
    //            game_state->num_players + 1, argv[i], p->pos_x, p->pos_y);

    //     game_state->num_players++;
    //     i++;
    

    // if (game_state->num_players == 0) {
    //     perror("Error: Se requiere al menos un jugador\n");
    //     exit(EXIT_FAILURE);
    // }

    return i; 
}


void arg_handler(int argc, char ** argv, GameState * game_state){
    if(argc > MAX_ARG_COUNT){
        perror("too many arguments");
        exit(EXIT_FAILURE);
    }

    if(argc < 2){
        perror("Error: At least one player must be specified using -p.");
        exit(EXIT_FAILURE);
    }

    int i = 1;
    while(i < argc){
        if(strcmp(argv[i], "-p") == 0){
            i = process_players(argv, argc, i + 1, game_state);
        }
        if(strcmp(argv[i], "-v") == 0){
            
        }
        // if(strcmp(argv[i], "-w") == 0){
            
        // }
        // if(strcmp(argv[i], "-h") == 0){
            
        // }
        // if(strcmp(argv[i], "-d") == 0){
            
        // }
        // if(strcmp(argv[i], "-t") == 0){
            
        // }
    }
}

int main (int argc, char ** argv){
    //create SHMs
    GameState * game_state = createSHM("/game_state", sizeof(GameState), 0644); //size of GameState is defined on createSHM
    GameSync * sync = createSHM("/game_sync", sizeof(GameSync), 0666);

    arg_handler(argc, argv, game_state);

    return 0;
}



// int process_arg(ArgType type, char **argv, int *indx, int argc, GameState *game){
//     int value = -1;
//     char *path = NULL;

//     switch(type){
//         case ARG

//     }
// }