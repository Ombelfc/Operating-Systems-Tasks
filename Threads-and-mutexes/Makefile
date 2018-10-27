CC = gcc
CFLAGS = -std=gnu99 -Wall
LDLIBS = -lpthread -lm
TARGET = main
FILES = main.o

.PHONY: all clean

all: ${TARGET}

main: main.o
	${CC} -o main main.o  ${LDLIBS}

main.o: main.c
	${CC} -o main.o -c main.c ${C_FLAGS}
	
clean: 
	-rm -f ${FILES} ${TARGET}
