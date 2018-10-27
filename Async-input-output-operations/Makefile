CC = gcc
CFLAGS = -std=gnu99 -Wall
LDLIBS = -lpthread -lm -lrt
TARGET = main
FILES = main.o
  
.PHONY: all clean
  
all: ${TARGET}
  
main: main.o
	${CC} -o main main.o  ${LDLIBS} 

main.o: task.c
	${CC} -o main.o -c task.c ${C_FLAGS} 
	         
clean: 
	-rm -f ${FILES} ${TARGET}

