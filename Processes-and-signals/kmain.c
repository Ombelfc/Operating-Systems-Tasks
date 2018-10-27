/*
 * kmain.c
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

void siguser_handler(int sig){
	
	int i;
	for(i = 0; i<2000; i++){
		printf("%c", 'a' + rand() % ('z' - 'a' + 1));
	}
	printf("\n");
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
	
	int t = (rand() % 7) + 12;
	int s = (rand() % 4) + 6;
	printf("Child: [%d] starts with [%d] and [%d]\n", getpid(), t, s);
	
	for(int tt = t; tt>0; tt = sleep(t));
	
	if(sig_count == s){
		printf("[%d] is disarmed\n", getpid());
	}
	else{
		printf("[%d] does KABOOM\n", getpid());
		if(kill(getppid(), SIGUSR1)) ERR("kill");
	}
}

void create_children(int num_pros){
	
	int i;
	pid_t pid;
	
	for(i = 0; i<num_pros; i++){
		if((pid = fork()) < 0) ERR("Fork:");
		if(pid == 0){
			sethandler(sig_handler, SIGUSR2);
			child_work();
			exit(EXIT_SUCCESS);
		}
	}
}

void parent_work(sigset_t oldmask){
	
	int i;
	srand(time(NULL) * getpid());
	int s = (rand() % 4) + 6;
	printf("[PARENT] has s as [%d]\n", s);
	
	struct timespec t = {0, 0.5*1000000000};
	struct timespec t_rem = {0, 0};
	
	for(i = 0; i<s; i++){
		while((nanosleep(&t, &t_rem)) == -1){
			t = t_rem;
		}
		if(kill(0, SIGUSR2) < 0) ERR("kill");
	}
	
	int g = 0;
	while((g = wait(NULL)) > 0){
		//printf("Value: [%d]\n", g);
		sigsuspend(&oldmask);
	} 
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
	sethandler(siguser_handler, SIGUSR1);
	sethandler(SIG_IGN, SIGUSR2);
	
	sigemptyset(&mask);
	
	sigaddset(&mask, SIGUSR1);
	
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	
	create_children(num_pros);
	
	parent_work(oldmask);
	
	//while(wait(NULL) > 0);
	
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
	return EXIT_SUCCESS;
}
