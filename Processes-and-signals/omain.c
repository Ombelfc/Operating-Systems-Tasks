/*
 * omain.c
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

#define MAX 30
#define ERR(source) (perror(source), kill(0, SIGKILL),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))

/*********** SIGNAL HANDLING *************/

static unsigned int count = 0;
volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t sig_count = 0;

void sethandler(void (*f)(int), int sigNo){
	
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if(-1 == sigaction(sigNo, &act, NULL)) ERR("sigaction:");
}

void sig_handler(int sigNo){
	
	last_signal = sigNo;
	sig_count += 1;
}

void chsig_handler(int sig){
	
	printf("child [%d] is to terminate. [%d] sent.\n", getpid(), count);
	if(kill(getpid(), SIGTERM)) ERR("kill:");
}

void sigchld_handler(int sigNo){
	
	pid_t pid;
	
	for(;;){
		pid = waitpid(0, NULL, WNOHANG);
		if(pid == 0) return;
		if(pid <= 0){
			if(errno == ECHILD) return;
			ERR("waitpid");
		}
	}
}

/*************** CORE LOGIC ***************/

void child_work(){
	
	int i;
	srand(time(NULL) * getpid());
	
	int to_sleep = (rand() % (200 + 1 - 100)) + 100;
	printf("Child: [%d] sleeps [%d] ms\n", getpid(), to_sleep);
	
	struct timespec t = {0, to_sleep*1000};
	
	for(i = 0; ; i++){
		nanosleep(&t, NULL);
		if(kill(getppid(), SIGUSR1)) ERR("kill:");
		printf("Signal sent: '*' [%d]\n", i+1);
		count += 1;
	}
}

void create_children(int num_pros){
	
	int i;
	pid_t pid;
	
	for(i = 0; i<num_pros; i++){
		if((pid = fork()) < 0) ERR("Fork:");
		if(pid == 0){
			sethandler(chsig_handler, SIGUSR2);
			child_work();
			exit(EXIT_SUCCESS);
		}
	}
}

void parent_work(sigset_t oldmask){
	
	while(sig_count <= 100){
		last_signal = 0;
		while(last_signal != SIGUSR1){
			sigsuspend(&oldmask);
		}
		printf("Counter: [%d]\n", sig_count);
	}
	
	if(kill(0, SIGUSR2)) ERR("kill:");
	printf("[PARENT] terminates.\n");
}

/************ MAIN FUNC *************/

void USAGE(char *name){
	
	fprintf(stderr, "USAGE: [%s] [Number of children]\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc < 2) USAGE(argv[0]);
	
	sigset_t mask, oldmask;
	int num_pros = atoi(argv[1]);
	if(num_pros < 0) USAGE(argv[0]);
	
	sethandler(sigchld_handler, SIGCHLD);
	sethandler(sig_handler, SIGUSR1);
	sethandler(SIG_IGN, SIGUSR2);
	
	sigemptyset(&mask);
	
	sigaddset(&mask, SIGUSR1);
	
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	
	create_children(num_pros);
	
	parent_work(oldmask);
	
	while(wait(NULL) > 0);
	
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
	return EXIT_SUCCESS;
}

