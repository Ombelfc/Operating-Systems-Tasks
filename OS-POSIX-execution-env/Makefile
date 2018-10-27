CC = gcc
CFLAGS = -std=gnu99 -Wall
TARGET = main
FILES = main.o

.PHONY: all clean

all: ${TARGET}

main: main.o
	${CC} -o main main.o

main.o: Main.c
	${CC} -o main.o -c Main.c ${C_FLAGS}
	
clean: 
	-rm -f ${FILES} ${TARGET}
