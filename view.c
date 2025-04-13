#include "commonHeaders.h"
#include "utilities/sync.h"

// Colors
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"
#define BRIGHT_RED    "\033[1;31m"
#define BRIGHT_GREEN  "\033[1;32m"
#define RESET   "\033[0m"

const char* player_colors[] = {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, BRIGHT_RED, BRIGHT_GREEN
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t size = sizeof(GameState) - sizeof(int) + (width * height * sizeof(int));

    GameState *game = open_existing_shm("/game_state", size, O_RDONLY);
    GameSync *sync = open_existing_shm("/game_sync", sizeof(GameSync), O_RDWR);

    int *local_board = malloc(width * height * sizeof(int)); 
    if(local_board == NULL) { 
        perror("Failed to allocate memory for local board");
        exit(EXIT_FAILURE);
    }
    memset(local_board, 0, width * height * sizeof(int)); 

    if(!local_board) {
        perror("Failed to allocate memory for local board");
        exit(EXIT_FAILURE);
    }

    memset(local_board, 0, width * height * sizeof(int));

    printf("\n🎮 ==================== WELCOME TO CHOMCHAMPS ==================== 🎮\n");
 
    printf("Game Board (Width: %d, Height: %d)\n\n", width, height);

    while (!game->game_over) {
        sem_wait(&sync->print_needed);

       
        printf("\033[1;33m┌");
        for (int i = 0; i < width * 2; i++) printf("─");
        printf("┐\033[0m\n");

        for (int i = 0; i < game->height; i++) {
            printf("\033[1;33m│\033[0m");
            for (int j = 0; j < game->width; j++) {
                bool printed = false;
                
                
                for (unsigned int p = 0; p < game->num_players; p++) {
                    if (game->players[p].x == j && game->players[p].y == i) {
                        
                        local_board[i * game->width + j] = p + 1; 

                        printf("%s#" RESET " ", player_colors[p]);
                        printed = true;
                        break;
                    }
                }
                
                if (!printed) {
                    int cell = game->board[i * game->width + j];
                    int local_cell = local_board[i * game->width + j];
                    if (local_cell == 0) {
                        // Reward 
                        printf(GREEN "%d " RESET, cell);
                    } else if (local_cell > 0) {
                        printf("%s#" RESET " ", player_colors[local_cell - 1]);
                    } else {
                        
                        printf(WHITE "#" RESET " ");
                    }
                }
            }
            printf("\033[1;33m│\033[0m\n");
        }


       
        printf("\033[1;33m└");
        for (int i = 0; i < width * 2; i++) printf("─");
        printf("┘\033[0m\n\n");

        // Print player stats
        printf("Player Statistics:\n");
        printf("-----------------\n");
        for (unsigned int i = 0; i < game->num_players; i++) {
            Player *p = &game->players[i];
            printf("%sPlayer %d%s: Position (%d,%d) | Score: %d | Valid Moves: %d | Invalid Moves: %d | %s\n",
                   player_colors[i], i + 1, RESET, p->x, p->y, p->score, p->v_moves, p->inv_moves,
                   p->is_blocked ? "BLOCKED" : "ACTIVE");
        }
        printf("\n");
        
        sem_post(&sync->print_done);
    }

   
    printf("\n\n");  
    printf("🎮 ==================== GAME OVER ==================== 🎮\n\n");
    printf("Final Scores (Ranked):\n");
    
   
    typedef struct {
        int player_id;
        int score;
    } PlayerScore;
    
    PlayerScore scores[9];  // Max 9 players
    for (unsigned int i = 0; i < game->num_players; i++) {
        scores[i].player_id = i;
        scores[i].score = game->players[i].score;
    }
    
    // Sort scores 
    for (unsigned int i = 0; i < game->num_players - 1; i++) {
        for (unsigned int j = 0; j < game->num_players - i - 1; j++) {
            if (scores[j].score < scores[j + 1].score) {
                PlayerScore temp = scores[j];
                scores[j] = scores[j + 1];
                scores[j + 1] = temp;
            }
        }
    }
    
   
    int highest_score = scores[0].score;
    int winner_count = 0;
    for (unsigned int i = 0; i < game->num_players && scores[i].score == highest_score; i++) {
        winner_count++;
    }
    
    // Print winner(s) and scores 
    printf("\n🏆 ");
    if (winner_count == 1) {
        printf("WINNER: %sPlayer %d%s with %d points!", 
               player_colors[scores[0].player_id], 
               scores[0].player_id + 1,
               RESET,
               scores[0].score);
    } else {
        printf("TIE BETWEEN:");
        for (int i = 0; i < winner_count; i++) {
            printf(" %sPlayer %d%s", 
                   player_colors[scores[i].player_id],
                   scores[i].player_id + 1,
                   RESET);
            if (i < winner_count - 1) {
                printf(",");
            }
        }
        printf(" with %d points!", scores[0].score);
    }
    printf(" 🏆\n\n");
    
   
    for (unsigned int i = 0; i < game->num_players; i++) {
        int player_id = scores[i].player_id;
        printf("%d. %sPlayer %d%s: Score: %d\n", 
               i + 1,
               player_colors[player_id], 
               player_id + 1, 
               RESET, 
               scores[i].score);
    }
    
   
    printf("\n================================================================\n");

    
    sem_post(&sync->print_done);

    close_shm();
    return 0;
} 