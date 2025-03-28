#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>  
#include <time.h>

#define TRUE 1
#define MAX_ARG_COUNT 23
#define MIN_BOARD_SIZE 10
#define MAX_PLAYERS 9
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

int nano_delay = 10000;

typedef struct Player {
    char name[16];
    unsigned int score;
    unsigned int inv_moves;
    unsigned int v_moves;
    unsigned short pos_x, pos_y;
    pid_t player_pid;
    bool is_blocked;
} Player;

typedef struct {
    unsigned short width, height;
    unsigned int num_players;
    Player players[MAX_PLAYERS];
    bool game_over;
    int board[];
} GameState;

typedef struct {
    sem_t *print_needed;  // Cambiado a puntero para macOS
    sem_t *print_done;     // Cambiado a puntero para macOS
    sem_t *master_utd;     // Cambiado a puntero para macOS
    sem_t *game_state_change; // Cambiado a puntero para macOS
    sem_t *sig_var;        // Cambiado a puntero para macOS
    unsigned int readers;
} GameSync;

// Tu función createSHM ORIGINAL (sin cambios)
void *createSHM(char *name, size_t size, mode_t mode) {
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, mode); 
    if(fd == -1) {                                
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if(strcmp(name, "/game_state") == 0) {
        GameState temp_game;
        read(fd, &temp_game, sizeof(GameState));
        size = sizeof(GameState) + temp_game.width * temp_game.height * sizeof(int);
    }

    if(-1 == ftruncate(fd, size)) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if(p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}


void init_semaphores(GameSync *sync) {
  
    sync->print_needed = sem_open("/print_needed", O_CREAT, 0644, 0);
    sync->print_done = sem_open("/print_done", O_CREAT, 0644, 0);
    sync->master_utd = sem_open("/master_utd", O_CREAT, 0644, 1);
    sync->game_state_change = sem_open("/game_state_change", O_CREAT, 0644, 1);
    sync->sig_var = sem_open("/sig_var", O_CREAT, 0644, 1);
    

    if (sync->print_needed == SEM_FAILED || sync->print_done == SEM_FAILED ||
        sync->master_utd == SEM_FAILED || sync->game_state_change == SEM_FAILED ||
        sync->sig_var == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    sync->readers = 0;
}

void cleanup_semaphores(GameSync *sync) {
    sem_close(sync->print_needed);
    sem_unlink("/print_needed");
    sem_close(sync->print_done);
    sem_unlink("/print_done");
    sem_close(sync->master_utd);
    sem_unlink("/master_utd");
    sem_close(sync->game_state_change);
    sem_unlink("/game_state_change");
    sem_close(sync->sig_var);
    sem_unlink("/sig_var");
}

void process_players(char **argv, int argc, int idx, GameState *game) {
    game->num_players = 0;
    
    for (int i = idx; i < argc && argv[i][0] != '-'; i++) {
        if (game->num_players >= MAX_PLAYERS) {
            perror("Demasiados jugadores");
            exit(EXIT_FAILURE);
        }

        int pipe_fd[2];
        if(pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        
        Player *p = &game->players[game->num_players];
        strncpy(p->name, argv[i], sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
        
        pid_t pid = fork();
        if(pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if(pid == 0) { // proceso hijo (player)
            close(pipe_fd[1]); // cerrar extremo write
            dup2(pipe_fd[0], STDOUT_FILENO);
            close(pipe_fd[0]); // cerrar extremo read

            execl(argv[i], argv[i], NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        } else {
            close(pipe_fd[0]); // cerrar extremo read
            p->player_pid = pid;
            p->is_blocked = false;
            game->num_players++;
        }
    }
}

void arg_handler(int argc, char **argv, GameState *game) {
    int opt;
    opterr = 0;
    memset(game, 0, sizeof(GameState));
    game->width = DEFAULT_WIDTH;
    game->height = DEFAULT_HEIGHT;
    char *view_path = NULL;

    while((opt = getopt(argc, argv, "w:h:t:d:p:v:")) != -1) {
        switch(opt) {
            case 'w':
                game->width = atoi(optarg);
                if(game->width < MIN_BOARD_SIZE) {
                    perror("board size too small");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                game->height = atoi(optarg);
                if(game->height < MIN_BOARD_SIZE) {
                    perror("board size too small");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                process_players(argv, argc, optind - 1, game);
                break;
            case 'v':
                view_path = optarg;
                if(access(view_path, X_OK) == -1) {
                    perror("error to access view");
                    exit(EXIT_FAILURE);
                }
                printf("Vista inicializada: %s\n", view_path);
                break;
            case '?':
                if(optopt == 'w' || optopt == 'h' || optopt == 't' || optopt == 'd' || optopt == 'p') {
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
    
    if(game->width == 0 || game->height == 0) {
        perror("must specify board size");
        exit(EXIT_FAILURE);
    }
    if(game->num_players == 0) {
        perror("must specify number of players");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {    
    // Crear memoria compartida (usando tu función original)
    GameState *game = createSHM("/game_state", sizeof(GameState), 0644);
    GameSync *sync = createSHM("/game_sync", sizeof(GameSync), 0666);
    

    init_semaphores(sync);
    
    // Procesar argumentos
    arg_handler(argc, argv, game);
    
    // Inicializar tablero
    srand(time(NULL));
    for (int i = 0; i < game->width * game->height; i++) {
        game->board[i] = rand() % 5;
    }
    
    // Posicionar jugadores
    for (unsigned int i = 0; i < game->num_players; i++) {
        game->players[i].pos_x = (game->width / (game->num_players + 1)) * (i + 1);
        game->players[i].pos_y = game->height / 2;
        game->players[i].score = 0;
        game->players[i].inv_moves = 0;
        game->players[i].v_moves = 0;
        game->players[i].is_blocked = false;
    }
    
   
    
    // Limpieza
    cleanup_semaphores(sync);
    munmap(game, sizeof(GameState) + game->width * game->height * sizeof(int));
    munmap(sync, sizeof(GameSync));
    shm_unlink("/game_state");
    shm_unlink("/game_sync");
    
    return 0;
}