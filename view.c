// This is a personal academic project. Dear PVS-Studio, please check it. 
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com


#include "commonHeaders.h"
#include "utilities/sync.h"
#define SHM_game "/game_state"
#define SHM_SYNC "/game_sync"
#define TRUE 1
#define FALSE 0


int main(int argc, char * argv[]) {
    if(argc > 4) {
        perror("view recieved too many arguments");
        exit(EXIT_FAILURE);
    }
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t size = sizeof(GameState) + width * height * sizeof(int);

    GameState *game = open_existing_shm("/game_state", size, O_RDONLY);

    GameSync *sync = open_existing_shm("/game_sync", sizeof(GameSync), O_RDWR);

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
                    printf("# ");
                }
            }
            printf("\n");                      
        }
        puts("\n");
        for (unsigned int i = 0; i < game->num_players; i++){
            printf("player %d: x=%d y=%d; score: %d / %d / %d   is_blocked:%d\n", 
                i+1, game->players[i].x, game->players[i].y, game->players[i].score, game->players[i].v_moves, game->players[i].inv_moves, game->players[i].is_blocked);
        }
        sem_post(&(sync->print_done));
        printf("\n");
    }

    
    return 0;
}
