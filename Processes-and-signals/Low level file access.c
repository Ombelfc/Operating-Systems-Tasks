/*
 * Main03.c
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
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERR(source) (fprintf(stderr, "%s: %d\n", __FILE__,__LINE__),\
					 perror(source), kill(0, SIGKILL),\
					 exit(EXIT_FAILURE))
					
volatile sig_atomic_t sig_count = 0;

/*********** SIGNAL HANDLING ************/

void sethandler(void (*f)(int), int sigNo){
	
	struct sigaction act;
	
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	
	if(-1 == sigaction(sigNo, &act, NULL)) ERR("sigaction:");
}

void sig_handler(int sig){
	
	sig_count++;
}

/************* CORE LOGIC ***************/

void child_work(int s){
	
	struct timespec t = {0, s*10000};
	
	sethandler(SIG_DFL, SIGUSR1); 
	//default handling for SIGUSR1 by the child
	//terminates the process
	
	while(1){
		nanosleep(&t, NULL);
		if(kill(getppid(), SIGUSR1)) ERR("Kill:");
	}	
}

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
	
	return len;	
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
		
	return len;
}

void parent_work(int set, int size, char *name){
	
	int i, in, out;
	ssize_t count;
	
	char *buf = malloc(size); 
	if(!buf) ERR("malloc:");
	
	if((out = TEMP_FAILURE_RETRY(open(name, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0777))) < 0) ERR("open out:");	
	if((in = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY))) < 0) ERR("open in:");
	
	for(i = 0; i<set; i++){
		printf("time to read\n");
		if((count = bulk_read(in, buf, size)) < 0) ERR("read in");
		if((count = bulk_write(out, buf, count)) < 0) ERR("write out");
		if(TEMP_FAILURE_RETRY(fprintf(stderr, "Block of %ld bytes transferred. Signals RX: %d\n", count, sig_count)) < 0) ERR("fprintf:");
	}
	
	if(TEMP_FAILURE_RETRY(close(in))) ERR("close in:");
	if(TEMP_FAILURE_RETRY(close(out))) ERR("close out");
	
	free(buf);
	
	if(kill(0, SIGUSR1)) ERR("kill:");
}

/************ MAIN FUNC **************/

void usage(char *name){

	fprintf(stderr, "USAGE: [%s] [Interval], [Set], [Size], [Name] \n", name);
	exit(EXIT_FAILURE);	
}

int main(int argc, char **argv)
{
	if(argc != 5) usage(argv[0]);
	
	pid_t chpid;
	
	int s = atoi(argv[1]);
	int set = atoi(argv[2]);
	int size = atoi(argv[3]);
	char *name = argv[4];
	
	if(s <= 0 || set <= 0 || size <= 0) usage(argv[0]);
	
	sethandler(sig_handler, SIGUSR1);
	
	if((chpid = fork()) < 0) ERR("Fork:");
	if(0 == chpid){
		child_work(s);
	}
	else{
		parent_work(set, size*1024*1024, name);
		while(wait(NULL) > 0);
	}
	
	return EXIT_SUCCESS;
}
