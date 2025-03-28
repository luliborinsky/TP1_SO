CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread

all: master view player

master: master.c
	$(CC) $(CFLAGS) master.c -o master $(LDFLAGS)

player: player_test.c
	$(CC) $(CFLAGS) player_test.c -o player $(LDFLAGS)

view: view.o board.o
	$(CC) $(CFLAGS) view.o board.o -o view $(LDFLAGS)

view.o: view.c board.h
	$(CC) $(CFLAGS) -c view.c

board.o: board.c board.h
	$(CC) $(CFLAGS) -c board.c

clean:
	rm -f master view player *.o

run: all
	./master