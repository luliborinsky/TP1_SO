CC = gcc
CFLAGS = -Wall -Wextra -pthread -lrt

# Compilar ambos binarios: jugador y vista
all: player view

# Compilar el jugador (player)
player: player_test.c
	$(CC) $(CFLAGS) player_test.c -o player

# Compilar la vista, usando view.o y board.o
view: view.o board.o
	$(CC) $(CFLAGS) view.o board.o -o view

# Reglas intermedias: compilar .o
view.o: view.c board.h
	$(CC) $(CFLAGS) -c view.c

board.o: board.c board.h
	$(CC) $(CFLAGS) -c board.c

# Limpiar binarios y objetos
clean:
	rm -f player view *.o

# Regla para ejecutar el máster con tus binarios
run: all
	./ChompChamps_mac -w 10 -h 10 -t 5 -p ./player -v ./view -d 2

