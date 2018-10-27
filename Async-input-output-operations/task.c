/*
 * task.c
 * 
 * Copyright 2017 Omar <omar@omar-HP-Pavilion-x2-Detachable>
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <aio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

volatile sig_atomic_t received;
pthread_t main_TID;

void error(char *msg){
	
	perror(msg);
	exit(EXIT_FAILURE);
}

void usage(char *progname){
	
	fprintf(stderr, "%s workfile blocksize\n", progname);
	fprintf(stderr, "workfile - path to the file to work on\n");
	fprintf(stderr, "n - number of blocks\n");
	
	exit(EXIT_FAILURE);
}

void sigiohandler(int sig){
	
	received = 1;
}

void sethandler(void (*f)(int), int sig){
	
	struct sigaction sa;
	memset(&sa, 0x00, sizeof(struct sigaction));
	sa.sa_handler = f;
	if(sigaction(sig, &sa, NULL) == -1) error("Error setting signal handler");
}

off_t getfilelength(int fd){
	
	struct stat buf;
	if (fstat(fd, &buf) == -1) error("Cannot fstat file");
	
	return buf.st_size;
}

void read_completed(sigval_t sigval){
	
	struct aiocb *req;
	req = (struct aiocb *) sigval.sival_ptr;
	
	if(aio_error(req) == 0){
		int ret = aio_return(req);
		if(ret > 0){
			char *buf = (char *) req->aio_buf;
			printf("[CONTENT] : \n %s \n", buf);
			while(*buf){
				if(!isalpha(*buf)){
					*buf = ' ';
				}
				buf++;
			}
			printf("[CONTENT CHANGED] : \n %s \n", (char *) req->aio_buf);
		}
		else{
			printf("Read 0 bytes\n");
		}
	}
	else{
		error("aio_error error");
	}
	
	if(pthread_kill(main_TID, SIGIO) != 0) error("pthread_kill");
}

void fillstructs(struct aiocb *aiocbs, char **buffer, int fd, int n, int blocksize){
	
	int current = 0;
	
	for(int i = 0; i<n; i++){
		memset(&aiocbs[i], 0, sizeof(struct aiocb));
		aiocbs[i].aio_fildes = fd;
		aiocbs[i].aio_offset = current;
		aiocbs[i].aio_nbytes = blocksize;
		aiocbs[i].aio_buf = (void *) buffer[i];
		aiocbs[i].aio_sigevent.sigev_notify = SIGEV_THREAD;
		aiocbs[i].aio_sigevent.sigev_notify_function = read_completed;
		aiocbs[i].aio_sigevent.sigev_notify_attributes = NULL;
		aiocbs[i].aio_sigevent.sigev_value.sival_ptr = &aiocbs[i];
		
		current += blocksize;
	}	
	
	printf("Filling structs done!\n");
}

void suspend(struct aiocb *aiocbs){
	
	struct aiocb *aiolist[1];
	aiolist[0] = aiocbs;
	
	while(aio_suspend((const struct aiocb *const *) aiolist, 1, NULL) == -1){
		if (errno == EINTR) continue;
			error("Suspend error");
	}
        
	if (aio_error(aiocbs) != 0) error("Suspend error");
        
    if (aio_return(aiocbs) == -1) error("Return error");
}

void writedata(struct aiocb *aiocbs){
	
	if (aio_write(aiocbs) == -1) error("Cannot write");
	printf("Data written!\n");
}

void start_work(struct aiocb *aiocbs, char **buffer, int n, int blocksize, int fd){
	
	for(int i = 0; i<n; i++){
		if(aio_read(&aiocbs[i]) == -1) error("Cannot read");
		
		while(aio_error(&aiocbs[i]) == EINPROGRESS);
		sleep(3);
		
		while(!received);
		printf("Signal received : %d\n", received);
		
		suspend(&aiocbs[i]);
	
		writedata(&aiocbs[i]);
		received = 0;
	}
	
	/*if(aio_read(&aiocbs[0]) == -1) error("Cannot read");
	
	while(aio_error(&aiocbs[0]) == EINPROGRESS);
	sleep(3);
	
	while(!received);
	printf("Signal received : %d\n", received);
	
	suspend(&aiocbs[0]);
	
	writedata(&aiocbs[0]);*/
	
	if (TEMP_FAILURE_RETRY(fsync(fd)) == -1) error("Error running fsync");
}

int main(int argc, char **argv)
{
	main_TID = pthread_self();
	
	char *filename; 
	int fd, n, blocksize;
	
	if (argc != 3) usage(argv[0]);
	
	filename = argv[1];
	n = atoi(argv[2]);
	
	if (n < 2) return EXIT_SUCCESS;
	
	struct aiocb aiocbs[n+1];
	char *buffer[n];
	
	received = 0;
	
	sethandler(sigiohandler, SIGIO);
	if((fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1) error("Cannot open file");
	
	blocksize = (getfilelength(fd) - 1) / n;
	
	printf("File size :  %ld\n", getfilelength(fd));
	
	for(int i = 0; i<n; i++){
		if ((buffer[i] = (char *) calloc (blocksize, sizeof(char))) == NULL) error("Cannot allocate memory");
	}
	
	int current = 0;
	int i = 0;
	while(i < n){
		printf("Offset : %d | Size : %d\n", current, blocksize);
		current += blocksize;
		i++;
	}
	
	fillstructs(aiocbs, buffer, fd, n, blocksize);
	
	start_work(aiocbs, buffer, n, blocksize, fd);
	
	for(int i = 0; i<n; i++){
		free(buffer[i]);
	}
	
	if (TEMP_FAILURE_RETRY(close(fd)) == -1) error("Cannot close file");
	
	return EXIT_SUCCESS;
}
