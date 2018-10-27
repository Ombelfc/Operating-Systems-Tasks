/*
 * Main02.c
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


volatile sig_atomic_t last_signal = 0;

/************* SIGNAL HANDLING **************/

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

/*************** MAIN LOGIC *****************/

void child_work(int m, int n){
	
	int count = 0;
	struct timespec t = {0, m*100000};
	
	while(1){
		for(int i = 0; i < n; i++){
			nanosleep(&t, NULL);
			if(kill(getppid(), SIGUSR1)) ERR("kill:");
		}
		sleep(1);
		nanosleep(&t, NULL);
		if(kill(getppid(), SIGUSR2)) ERR("kill:");
		count++;
		printf("[%d] sent [%d] SIGUSR2\n", getpid(), count);
	}
	
	printf("Child: [%d] terminates. Signal count: [%d]\n", getpid(), count);
}

void parent_work(sigset_t oldmask){
	
	int count = 0;
	while(1){
		last_signal = 0;
		while(last_signal != SIGUSR2){
			sigsuspend(&oldmask);
		}
		count++;
		printf("[PARENT] received [%d] SIGUSR2\n", count);
	}
}

/*************** MAIN FUNCTION *************/

void usage(char *name){
	
	fprintf(stderr, "USAGE: [%s], [Time before SIGUSR1], [TIme before SIGUSR2] \n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc != 3) usage(argv[0]);
	
	pid_t pid;
	sigset_t mask, oldmask;
	
	int m = atoi(argv[1]);
	int n = atoi(argv[2]);
	
	if(m <= 0 || n <= 0) usage(argv[0]);
	
	sethandler(sigchld_handler, SIGCHLD);
	sethandler(sig_handler, SIGUSR1);
	sethandler(sig_handler, SIGUSR2);
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	
	if((pid = fork()) < 0) ERR("fork:");
	if(0 == pid) child_work(m, n);
	else{
		parent_work(oldmask);
		while(wait(NULL) > 0);
	}
	
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
	return EXIT_SUCCESS;
}
