CC = gcc
CFLAGS = -Wall -Wextra -pthread -lrt

all: player view

player: player.c
	$(CC) $(CFLAGS) player.c -o player.out

view: view.c
	$(CC) $(CFLAGS) view.c -o view.out

clean:
	rm view.out player.out

run: all
	./ChompChamps -p player.out -v view.out