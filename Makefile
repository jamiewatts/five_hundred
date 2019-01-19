CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99 -pthread
DEPS = game.h networking.h pending.h

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: client499 serv499

clean:
	rm -f client499 serv499
	rm -f client.o game.o networking.o server.o pending.o
	rm -rf res.*
	rm -rf deleteme.*
	rm -rf testres.*

client499: client.o game.o networking.o
	$(CC) $(CFLAGS) -o $@ $^

serv499: server.o game.o networking.o pending.o
	$(CC) $(CFLAGS) -o $@ $^
