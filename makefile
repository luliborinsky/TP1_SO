CC = gcc
CFLAGS = -Wall -Wextra -pthread -lrt

all: master player view

master: master.c
	$(CC) $(CFLAGS) master.c -o master.out

player: player.c
	$(CC) $(CFLAGS) player.c -o player.out

view: view.c
	$(CC) $(CFLAGS) view.c -o view.out

clean:
	rm master.out view.out player.out

run: all
	./ChompChamps_amd -p player.out -v view.out