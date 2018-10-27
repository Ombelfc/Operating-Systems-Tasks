/*
 * main.c
 * 
 * Copyright 2017 280944 berraj omar <berrajo@p21920.mini.pw.edu.pl>
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
					
int count1 = 1;	
int h = 0;	
volatile sig_atomic_t last_signal = 0;

void sethandler(void (*f)(int), int sigNo){
	
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	
	if(-1 == sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sig_handler(int sigNo){
	
	last_signal = sigNo;
}
						
ssize_t bulk_read(int fd, char *buf, size_t count){
        ssize_t c;
        ssize_t len=0;
        do{
                c=TEMP_FAILURE_RETRY(read(fd,buf,count));
                if(c<0) return c;
                if(c==0) return len; //EOF
                //printf("[CONTENT] %s\n", buf);
                buf+=c;
                len+=c;
                count-=c;
        }while(count>0);
        return len ;
}
											
void child_work(char *argv[], sigset_t oldmask){
	
	int i;
	int in, out;
	ssize_t count;
	char buf[255];
	
	srand(time(NULL)*getpid());
	
	int b = (rand() % (100 -20 + 1)) + 20; 
	
	printf("[FILE]: %s, [B]: %d\n", argv[count1], b);
	
	/*char *buf = malloc(b);
	if(!buf) ERR("malloc:");*/
	
	char* name = argv[count1];
	
	if((out = TEMP_FAILURE_RETRY(open(name, O_RDONLY))) < 0) ERR("open:");
	
	while(1){
		
		printf("I'm here\n");
		while(last_signal != SIGUSR2){
			sigsuspend(&oldmask);
		}
		printf("i'm down here\n");	
		for(;;){
		
			fprintf(stderr, "--- %d Start ---\n", getpid());
		
			if((count = bulk_read(out, buf, b)) < 0) ERR("read:");
			if (count == 0) break;
			else {
				buf[count] = '\0';
				fprintf(stderr, "%s\n", buf);
			}
        
			fprintf(stderr, "--- %d End ---\n", getpid());
		}
			
		last_signal = 0;
	}
		
	if(TEMP_FAILURE_RETRY(close(out)))ERR("close");
}			
					
void create_children(int nums, char *argv[], sigset_t oldmask){
	
	pid_t pid;
	
	while(nums-- > 1){
		switch(fork()){
			case 0:
				sethandler(sig_handler, SIGUSR2);
				child_work(argv, oldmask);
				exit(EXIT_SUCCESS);
			case -1:
				ERR("Fork:");
		}
		count1++;
	}
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	
	int N = (rand() % (900 - 100 + 1)) + 100;
	
	
	sethandler(SIG_IGN, SIGUSR2);
	
	sigset_t mask, oldmask;
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);
	
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	
	create_children(argc, argv, oldmask);
	
	printf("[PARENT]: %d\n", N);
	
	while(1){
		printf("Parent sending\n");
		sleep(N/1000);
		if(kill(0, SIGUSR2)) ERR("Kill:");
	}
	while(wait(NULL) > 0){}
	
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
	return EXIT_SUCCESS;
}
