CC = gcc 

CFLAGS = -Wall -g 

all: parser.o
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/vlc obj/parser.o 

parser.o: interpreter/parser.c interpreter/parser.h
	mkdir -p obj 
	$(CC) $(CFLAGS) -c interpreter/parser.c -o obj/parser.o

clean: 
	rm -f obj/*.o bin/vlc

