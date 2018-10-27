/*
 * Main.c
 * 
 * Copyright 2017 root <root@omar-HP-Pavilion-x2-Detachable>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#define ERR(source) (perror(source), kill(0, SIGKILL),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))

void child_work(int num){
	
	srand(time(NULL) * getpid());
	
	int secs_to_sleep = (rand() % (10 + 1 - 5)) + 5;
	sleep(secs_to_sleep);
	printf("Process number %d has terminated\n", getpid());	
}

void create_children(int num){
	
	pid_t pid;
	
	for(num; num >= 0; num--){
		
		if((pid = fork()) < 0) ERR("Fork:");
		if(pid == 0){
			child_work(num);
			exit(EXIT_SUCCESS);
		}
	}
}

void usage(char *name){
	
	fprintf(stderr, "Usage: %s num_of_processes > 0\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc != 2) usage(argv[0]);
	
	int num_of_processes;
	
	if((num_of_processes = atoi(argv[1])) <= 0) usage(argv[0]);
	
	create_children(num_of_processes);
	
	while(num_of_processes > 0){
			
		sleep(3);
		pid_t pid;
		
		for(;;){
			pid = waitpid(0, NULL, WNOHANG);
			if(pid > 0) num_of_processes--;
			else if(0 == pid) break;
			else if(0 >= pid) {
				if(ECHILD == errno) break;
				ERR("waitpid:");
			}
		}
		printf("PARENT: [%d] processes remain\n", num_of_processes);
	}
	
	return EXIT_SUCCESS;
}
