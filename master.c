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
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

int nano_delay = 10000;

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

void process_players(char ** argv, int argc, int idx, GameState * game){
    game->num_players = 0;
    pid_t player_pid;
    
    for (int i = idx; i < argc && argv[i][0] != '-'; i++) {
        if (game->num_players >= MAX_PLAYERS) {
            perror("Demasiados jugadores");
            exit(EXIT_FAILURE);
        }
        
        Player *p = &game->players[game->num_players];
        strncpy(p->name, argv[i], sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
        p->score = 0;
        p->inv_moves = 0;
        p->v_moves = 0;
        p->is_blocked = false;

        player_pid = fork();
        if(player_pid == -1){
            perror("player fork");
            exit(EXIT_FAILURE);
        } else {
            printf("player pid %d\n", player_pid);
        }


        printf("Jugador inicializado: %s\n", p->name);
        game->num_players++;
    }
}


void arg_handler(int argc, char ** argv, GameState * game){
    int opt;
    opterr = 0;
    memset(game, 0, sizeof(GameState));
    game->width = DEFAULT_WIDTH;
    game->height = DEFAULT_HEIGHT;
    char *view_path = NULL;
    //game->timeout = DEFAULT_TIMEOUT;
    //game->delay = DEFAULT_DELAY;


    while((opt = getopt(argc, argv, "w:h:t:d:p:v:")) != -1){
        switch(opt){
            case 'w':
                game->width = atoi(optarg);
                if(game->width < MIN_BOARD_SIZE){
                    perror("board size too small");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                game->height = atoi(optarg);
                if(game->height < MIN_BOARD_SIZE){
                    perror("board size too small");
                    exit(EXIT_FAILURE);
                }
                break;

            // case 't':
            //     game->timeout = atoi(optarg);
            //     if(game->timeout <= 0){
            //         perror("timeout must be positive");
            //         exit(EXIT_FAILURE);
            //     }
            //     break;
            // case 'd':
            //     game->delay = atoi(optarg);
            //     if(game->delay <= 0){
            //         perror("delay must be positive");
            //         exit(EXIT_FAILURE);
            //     }
            //     break;
            case 'p':
                process_players(argv, argc, optind - 1, game);
                break;
            case 'v':
                view_path = optarg;
                if(access(view_path, X_OK) == -1){
                    perror("error to access view");
                    exit(EXIT_FAILURE);
                }
                printf("Vista inicializada: %s\n", view_path);
                break;
                
            case '?':
                if(optopt == 'w' || optopt == 'h' || optopt == 't' || optopt == 'd' || optopt == 'p'){
                    perror("option requires an argument");
                } else {
                    perror("unknown option");
                }
                exit(EXIT_FAILURE);

            default:
                perror("bad usage");
                exit(EXIT_FAILURE);
        }   
    }
    if(game->width == 0 || game->height == 0){
        perror("must specify board size");
        exit(EXIT_FAILURE);
    }
    if(game->num_players == 0){
        perror("must specify number of players");
        exit(EXIT_FAILURE);
    }
}

int main (int argc, char ** argv){    
    //create SHMs
    GameState * game = createSHM("/game_state", sizeof(GameState), 0644); //size of GameState is defined on createSHM
    GameSync * sync = createSHM("/game_sync", sizeof(GameSync), 0666);

    arg_handler(argc, argv, game);
    return 0;
}



