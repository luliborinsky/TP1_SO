#include "gameLogic.h"


void init_board(GameState *game, unsigned int seed){
    srand(seed);

    size_t board_size = game->width * game->height;
    
    for(unsigned int i = 0; i < board_size; i++){
        game->board[i] = (rand() % 9) + 1;
    }

    int zones_per_row = (int)ceil(sqrt(game->num_players));
    int zone_width = game->width / zones_per_row;
    int zone_height = game->height / zones_per_row;

    for (unsigned int i = 0; i < game->num_players; i++) {
        int zone_col = i % zones_per_row;
        int zone_row = i / zones_per_row;

        int x_center = zone_col * zone_width + zone_width / 2;
        int y_center = zone_row * zone_height + zone_height / 2;

        game->players[i].x = x_center;
        game->players[i].y = y_center;
        game->board[y_center * game->width + x_center] = 0;
    }
}

void init_game_state(GameState * game, int width, int height){
    game->game_over = false;
    game->num_players = 0;
    game->width = width;
    game->height = height;
    if(game->height < DEFAULT_HEIGHT){
        perror("height must be higher than 9");
        exit(EXIT_FAILURE);
    }
    if(game->width < DEFAULT_WIDTH){
        perror("width must be higher than 9");
        exit(EXIT_FAILURE);
    }

    Player p;
    while(player_paths[game->num_players] != NULL && game->num_players < 10){
        strncpy(p.name, player_paths[game->num_players], sizeof(p.name) - 1);

        p.name[sizeof(p.name) - 1] = '\0';
        p.score = 0;
        p.inv_moves = 0;
        p.v_moves = 0;
        p.is_blocked = false;
        game->players[game->num_players] = p;
        game->num_players++;
    }
}



bool all_players_blocked(GameState * game){
    for(unsigned int i = 0; i < game->num_players; i++){
        if(!game->players[i].is_blocked) return false;
    }
    return true;
}

int handle_moves(GameState *game, struct timeval *timeout, unsigned char * player_moves) {
    int ready = select(highest_fd, &read_fds, NULL, NULL, timeout);

    if (ready == -1) {
        perror("select error\n");
        return -1;
    } else if (ready == 0) {
        printf("Timeout reached. No input received.\n");
        return -1;
    }

    for (unsigned int i = 0; i < game->num_players; i++) {
        int fd = player_pipes[i][0];
        if (FD_ISSET(fd, &read_fds)) {
            unsigned char move;

            if (read(fd, &move, sizeof(move)) <= 0) {
                perror("read from player\n");
                continue;
            }

            player_moves[i] = move;
        }
    }

    return 0;
}

bool valid_move(GameState * game, int x, int y){
    if(x < 0 || x >= game->width || y < 0 || y >= game->height){
        return false;
    }
    if(game->board[y * game->width + x] > 0) {
        return true;
    }
    return false;
}

bool has_available_moves(GameState * game, int player_idx){
    int dir_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dir_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    for (int i = 0; i < 8; i++){
        int new_x = game->players[player_idx].x + dir_x[i];
        int new_y = game->players[player_idx].y + dir_y[i];

        if(valid_move(game, new_x, new_y)) return true;
    }

    return false;
}

void process_player_move(GameState * game, int player_idx, unsigned char move){
    int dir_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dir_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    int new_x = game->players[player_idx].x + dir_x[move];
    int new_y = game->players[player_idx].y + dir_y[move];

    if(!has_available_moves(game, player_idx)){
        game->players[player_idx].is_blocked = true;
        return;
    }

    if(!valid_move(game, new_x, new_y)){
        game->players[player_idx].inv_moves++;
        return;
    }

    game->players[player_idx].v_moves++;
    game->players[player_idx].x = new_x;
    game->players[player_idx].y = new_y;
    game->players[player_idx].score += game->board[new_y + game->width + new_x];
    game->board[new_y * game->width + new_x] = 0;
}

void initial_print(GameState * game, int width, int height, int delay, int timeout, int seed, char * view_path){
    printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %d\n",
            width, height, delay, timeout, seed, view_path, game->num_players);
    for(unsigned int i = 0; i<game->num_players; i++){
        printf("\t%s\n", game->players[i].name);
    }
    sleep(1);
}

void print_final_state(GameState * game, char * view_path, pid_t view_pid){
    printf("game is %s over\n", game->game_over? "" : "NOT");

    int wstatus;
    pid_t returned_pid;
    Player player;
    for(unsigned int i = 0; i < game->num_players; i++){
        player = game->players[i];
        returned_pid = waitpid(player.pid, &wstatus, 0);
        if(returned_pid == -1){
            printf("WAITPID failed on player (%s) with pid %d\n", player.name, player.pid);
        }
        if(WIFEXITED(wstatus)){
            printf("Player %d %s exited with status (%d), score %d / %d / %d %s\n"
            , i+1, player.name, wstatus, player.score, player.v_moves, player.v_moves, player.is_blocked? "BLOCKED" : "NOT BLOCKED");
        }
    }

    if(view_path != NULL){
        returned_pid = waitpid(view_pid, &wstatus, 0);
        if(returned_pid == -1){
            printf("WAITPID failed on view with pid %d\n", view_pid);
        }
        if(WIFEXITED(wstatus)){
            printf("View exited with status (%d)\n", wstatus);
        }
    }
}

