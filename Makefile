CC=gcc
OUTPUT=siv
LIBS=pkg-config --cflags --libs gtk+-2.0

target: all

all: 
	$(CC) main.c -o $(OUTPUT) `$(LIBS)`

