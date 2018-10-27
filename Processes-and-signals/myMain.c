/*
 * myMain.c
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

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ERR(source) (perror(source), kill(0, SIGKILL),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))
		
/*********** SIGNAL HANDLING ************/

volatile sig_atomic_t sig_count = 0;

void sethandler(void (*f)(int), int sigNo){
	
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	
	if(-1 == sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sig_handler(int sigNo){
	
	sig_count += 1;	
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
		
/*********** CORE LOGIC **************/		
		
ssize_t bulk_read(int fd, char *buf, size_t count){
	
	ssize_t c;
	ssize_t len = 0;
	
	do{
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if(c < 0) return c;
		if(c == 0) return len; //EOF
		buf += c;
		len += c;
		count -= c;
	} while(count > 0);
	
	return len ;
}

ssize_t bulk_write(int fd, char *buf, size_t count){
	
	ssize_t c;
	ssize_t len = 0;
	
	do{
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if(c < 0) return c;
		buf += c;
		len += c;
		count -= c;
	} while(count > 0);
	
	return len ;
}		
		
void create_file(int size, char *name){
	
	int in, out;
	ssize_t count;
	
	char *buf = malloc(size);
	if(!buf) ERR("malloc:");
	
	if((out = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0) ERR("open:");
	if((in = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY))) < 0) ERR("open:");
	
	if((count = bulk_read(in, buf, size)) < 0) ERR("read:");
	if((count = bulk_write(out, buf, count)) < 0) ERR("read:");
	if(TEMP_FAILURE_RETRY(fprintf(stderr, "Block of %ld bytes transfered. Signals RX:%d\n", count, sig_count)) < 0) ERR("fprintf");
	
	if(TEMP_FAILURE_RETRY(close(in)))ERR("close");
	if(TEMP_FAILURE_RETRY(close(out)))ERR("close");
	
	free(buf);
	
	if(kill(0, SIGUSR2)) ERR("Kill:");
}
		
void parent_work(int size, char *name, sigset_t oldmask){
	
	srand(time(NULL));
	
	int to_sleep = (rand() % (15 - 5 + 1)) + 5;
	
	for(int tt = to_sleep; tt > 0; tt = sleep(to_sleep));
	
	while(1){
	
		sigsuspend(&oldmask);
	
		if(sig_count >= to_sleep){
			printf("[PARENT] Mission confirmed.\n");
			create_file(size, name);
			break;
		}
		else{
			printf("[PARENT] Signal count missed the target. Mission Abort.\n");
		}
	}	
	
	printf("[SIGUSR1] count: [%d]\n", sig_count);
}
		
void child_work(){
	
	srand(time(NULL) * getpid());
	
	int i;
	int to_sleep = (rand() % (5 - 2 + 1)) + 2;
	
	while(1){
		
		for(i = to_sleep; i > 0; i = sleep(to_sleep));
		printf("[CHILD] [%d] sleeps [%d]\n", getpid(), to_sleep);
	
		if(kill(getppid(), SIGUSR1)) ERR("kill:");
	}
}		
		
void create_children(int num_pros){
	
	pid_t pid;
	
	while(num_pros-- > 0){
		switch(fork()){
			case 0: 
				sethandler(SIG_DFL, SIGUSR2);
				child_work();
				exit(EXIT_SUCCESS);
			case -1:
				ERR("Fork:");
		}
	}
}
		
/************ MAIN FUNC ************/		
		
void USAGE(char *name){
	
	fprintf(stderr, "USAGE: [%s] [Number of children] [Size of file] [Name of the file]\n", name);
	exit(EXIT_FAILURE);
}		
					
int main(int argc, char **argv)
{
	if(argc < 4) USAGE(argv[0]);
	
	sethandler(sigchld_handler, SIGCHLD);
	sethandler(sig_handler, SIGUSR1);
	sethandler(SIG_IGN, SIGUSR2);
	
	sigset_t mask, oldmask;
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	
	int num_pros, size;
	char *name;
	
	if((num_pros = atoi(argv[1])) < 1) USAGE(argv[0]);
	if((size = atoi(argv[2])) < 1) USAGE(argv[0]);
	name = argv[3];
	
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	
	create_children(num_pros);
	
	parent_work(size*1024*1024, name, oldmask);
	
	while(wait(NULL) > 0);
	
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
	return EXIT_SUCCESS;
}

