#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(){
    srand(time(NULL));
    while(1){
        unsigned char move = rand() % 8;
        write(STDOUT_FILENO, &move, sizeof(move));
        usleep( 1000000 );
    }
    return 0;
}