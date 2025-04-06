#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "master.h"




bool validate_move(const Gamegame *game, int player_idx, unsigned char move) {
    if (move >= 8) {
        return false; 
    ]
    if (game->players[player_idx].blocked) {
        return false;
    }
    
    const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    int curr_x = game->players[player_idx].x + dx[move];
    int curr_y = game->players[player_idx].y + dy[move];

   
    if (curr_x < 0 || curr_x >= game->width || curr_y < 0 || curr_y >= game->height) {
        return false;
    }

    
    return game->board[curr_y * game->width + curr_x] > 0;


bool has_available_moves(const Gamegame *game, int player_idx) {
    if (game->players[player_idx].blocked){
        return false;
    } 

    // Probar las 8 direcciones posibles
    for (unsigned char move = 0; move < 8; move++) {
        if (validate_move(game, player_idx, move)) {
            return true;
        }
    }
    return false;
}

void process_move(Gamegame *game, int player_idx, unsigned char move) {
    if (!validate_move(game, player_idx, move)) {
        game->players[player_idx].invalid_moves++;
        return;
    }

    const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    int curr_x = game->players[player_idx].x + dx[move];
    int curr_y = game->players[player_idx].y + dy[move];

    
    int reward = game->board[curr_y * game->width + curr_x];
    
    // Actualizar estado del juego
    game->players[player_idx].score += reward;
    game->players[player_idx].valid_moves++;
    game->players[player_idx].x = curr_x;
    game->players[player_idx].y = curr_y;
    
    
    game->board[new_y * game->width + new_x] = -player_idx;
}

// int determine_winner(const Gamegame *game) {
//     int winner = -1;
//     int max_score = -1;
//     int min_valid_moves = INT_MAX;
//     int min_invalid_moves = INT_MAX;

//     for (int i = 0; i < game->num_players; i++) {
//         if (game->players[i].score > max_score) {
//             max_score = game->players[i].score;
//             winner = i;
//             min_valid_moves = game->players[i].valid_moves;
//             min_invalid_moves = game->players[i].invalid_moves;
//         } 
//         else if (game->players[i].score == max_score) {
//             if (game->players[i].valid_moves < min_valid_moves) {
//                 winner = i;
//                 min_valid_moves = game->players[i].valid_moves;
//                 min_invalid_moves = game->players[i].invalid_moves;
//             } 
//             else if (game->players[i].valid_moves == min_valid_moves) {
//                 if (game->players[i].invalid_moves < min_invalid_moves) {
//                     winner = i;
//                     min_invalid_moves = game->players[i].invalid_moves;
//                 }
//             }
//         }
//     }

//     return winner;
// }

bool check_game_over(Gamegame *game) {
    // Verificar si todos los jugadores están bloqueados
    for (int i = 0; i < game->num_players; i++) {
        if (!game->players[i].blocked && has_valid_moves(game, i)) {
            return false;
        }
    }
    game->game_over = true;
    return true;
}