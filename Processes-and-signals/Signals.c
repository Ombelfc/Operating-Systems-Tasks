/*
 * Main01.c
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
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define ERR(source) (perror(source), kill(0, SIGKILL),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))

/*************** SIGNAL HANDLING ***************/

volatile sig_atomic_t last_signal = 0; 

void sethandler(void (*f)(int), int sigNo){
		
	struct sigaction act;
	
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f; //pointer to a signal handling function 
	
	if(-1 == sigaction(sigNo, &act, NULL)) ERR("sigaction:"); //set the signal to be handled by the signal handling function
}

void sig_handler(int sig){
	
	printf("[%d] received signal [%d]\n", getpid(), sig);
	last_signal = sig;	
}

void sigchld_handler(int sig){
	
	pid_t pid;
	
	for(;;){
		pid = waitpid(0, NULL, WNOHANG);
		if(pid == 0) return;
		if(pid <= 0) {
			if(errno == ECHILD) return;
			ERR("waitpid:");
		}
	}
}

/*************** CORE IMPLEMENTATION ****************/

void parent_work(int k, int p, int ite){
	
	struct timespec tk = {k, 0};
	struct timespec tp = {p, 0};
	
	sethandler(sig_handler, SIGALRM);
	alarm(ite * 10);
	
	while(last_signal != SIGALRM){
		nanosleep(&tk, NULL);
		if(kill(0, SIGUSR1) < 0) ERR("kill:");
		nanosleep(&tp, NULL);
		if(kill(0, SIGUSR2) < 0) ERR("kill:");
	}
	printf("PARENT terminates\n");
}

void child_work(int ite){
	
	int i;
	srand(getpid());
	
	int to_sleep = rand() % 6 + 5;
	
	while(ite-- > 0){
		for(i = to_sleep; i > 0; i = sleep(i)); 
		//Sleep, once interrupted by signal handling, returns the remaining time. Restart is a must.
		if(last_signal == SIGUSR1) printf("Success [%d]\n", getpid());
		else printf("Failure [%d]\n", getpid());
	}
	printf("[%d] Terminates \n", getpid());
}

void create_children(int num, int ite){
	
	while(num-- > 0){
		switch(fork()){
			case 0: sethandler(sig_handler, SIGUSR1); 
					sethandler(sig_handler, SIGUSR2);
					child_work(ite);
					exit(EXIT_SUCCESS);
			case -1: perror("Fork:");
					 exit(EXIT_FAILURE);
		}
	}
}

/***************** MAIN ****************/

void usage(char *name){
	
	fprintf(stderr, "USAGE: %s, [Children], [First sig delay], [Second sig delay], [Iterations]\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	
	if(argc != 5) usage(argv[0]);
	
	int num = atoi(argv[1]);
	int sig01_delay = atoi(argv[2]);
	int sig02_delay = atoi(argv[3]);
	int iterations = atoi(argv[4]);
	
	if(num <= 0 || sig01_delay <= 0 || sig02_delay <= 0 || iterations <= 0) usage(argv[0]);
	
	sethandler(sigchld_handler, SIGCHLD); //sigchld_handler handles SIGCHLD signal 
	sethandler(SIG_IGN, SIGUSR1); //Parent ignores SIGUSR1
	sethandler(SIG_IGN, SIGUSR2); //Parent ignores SIGUSR2
	
	create_children(num, iterations); 
	
	parent_work(sig01_delay, sig02_delay, iterations);
	
	while(wait(NULL) > 0);
	
	return 0;
}

