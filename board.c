#include <stdio.h>
#include "board.h"

// array para guardar la posiciones del cuerpo
#define MAX_BODY_SIZE 100
int body_x[MAX_BODY_SIZE];
int body_y[MAX_BODY_SIZE];
int body_length = 0;

void printBoard(GameState *game) {
    int width  = game->width;
    int height = game->height;
    int *board = game->board;

    // obtengo posicion
    int head_x = -1, head_y = -1;
    for (unsigned int p = 0; p < game->num_players; p++) {
        head_x = game->players[p].x;
        head_y = game->players[p].y;
    }

    // guardo la cabeza
    if (body_length < MAX_BODY_SIZE) {
        body_x[body_length] = head_x;
        body_y[body_length] = head_y;
        body_length++;
    }

    // línea superior del tablero
    printf("\033[1;33m┌");
    for (int j = 0; j < width; j++) {
        printf("────");
    }
    printf("┐\033[0m\n");

    for (int i = 0; i < height; i++) {
        printf("\033[1;33m│\033[0m"); // borde izq

        for (int j = 0; j < width; j++) {
            // verifisco si es la cabeza
            if (i == head_y && j == head_x) {
                printf("\033[1;31m O  \033[0m");  // cabeza en rojo
            }
            // verifico si es parte del cuerpo guardado en la lista
            else {
                int is_body = 0;
                for (int k = 0; k < body_length; k++) {
                    if (body_x[k] == j && body_y[k] == i) {
                        is_body = 1;
                        break;
                    }
                }
                if (is_body) {
                    printf("\033[1;32m o  \033[0m");  // cuerpo en verde
                }
                else if (board[i * width + j] > 0) {
                    printf("\033[1;33m %-2d \033[0m", board[i * width + j]);  // recompensa en amarillo
                }
                else {
                    printf("\033[1;32m .  \033[0m");  // celda vacía en verde
                }
            }
        }

        printf("\033[1;33m│\033[0m\n"); // borde derecho
    }

    // línea inferior del tablero
    printf("\033[1;33m└");
    for (int j = 0; j < width; j++) {
        printf("────");
    }
    printf("┘\033[0m\n");
}
