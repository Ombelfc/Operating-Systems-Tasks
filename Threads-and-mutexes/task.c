/*
 * main.c
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


#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>

#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

typedef struct timespec timespec_t;
typedef unsigned int UINT;
typedef struct args {
		
	pthread_t tid;
	float *arr;
	float *res;
	bool *c;
	pthread_mutex_t *mxs;
	pthread_mutex_t *cond;
	UINT seed;
        
} args_t;


void ReadArguments(int argc, char** argv, int *threads, int *size);
void *thread_work(void *ths);
void msleep(UINT milisec);

int main(int argc, char **argv)
{
	int threads, size;
	
	ReadArguments(argc, argv, &threads, &size);
	
	float arr[size];
	float res[size];
	pthread_mutex_t mx[size];
	pthread_mutex_t cond[size];
	bool c[size];
	
	srand(time(NULL));
	for(int i = 0; i < size; i++){
		c[i] = false;	
		res[i] = (float) 0;
		arr[i] = (float) (rand() % 60) + 1;
		if(pthread_mutex_init(&mx[i], NULL)) ERR("Couldn't initialize mutex!");
		if(pthread_mutex_init(&cond[i], NULL)) ERR("Couldn't initialize mutex!");
		//printf("%f : \n", arr[i]);
	}
	
	args_t* ths = (args_t*) malloc(sizeof(args_t) * threads);
	if (ths == NULL) ERR("Malloc error for estimation arguments!");
	
	for(int i = 0; i <= threads; i++){
		ths[i].seed = (UINT) rand();
		ths[i].arr = arr;
		ths[i].res = res;
		ths[i].mxs = mx;
		ths[i].c = c;
		ths[i].cond = cond;
	}
	
	printf("Threads : \n");
	for (int i = 0; i < threads; i++) {
		int err = pthread_create(&(ths[i].tid), NULL, thread_work, &ths[i]);
		if (err != 0) ERR("Couldn't create thread");
	}
	
	for (int i = 0; i < threads; i++) {
		int err = pthread_join(ths[i].tid, NULL);
		if (err != 0) ERR("Can't join with a thread");
	}
	
	printf("[MAIN] : \n");
	for(int i = 0; i<size; i++){
		printf("[VALUE] : %f, [RESULT] : %f\n", arr[i], res[i]);
	}
	
	free(ths);
	exit(EXIT_SUCCESS);
}

void *thread_work(void *ths){
	
	args_t *th = ths;
	
	int index;
	int count = 7;
	
	do{	
		index = rand_r(&th->seed) % 7;
		
		pthread_mutex_lock(&th->mxs[index]);
		
		th->res[index] = sqrt(th->arr[index]); 
		printf("sqrt(%f) : %f\n", th->arr[index], th->res[index]);
		
		pthread_mutex_unlock(&th->mxs[index]);
		
		pthread_mutex_lock(&th->cond[index]);
		
		th->c[index] = true; 
	
		pthread_mutex_unlock(&th->cond[index]);
		
		count--;
		
	} while(&th->c[index] == false || count > 0);
	
	//printf("%d : \n", th->arr[index]);
	//printf("*\n");
	
	msleep(100);
	return NULL;
}

void ReadArguments(int argc, char** argv, int *threads, int *size) {
	
	*threads = 3;
	*size = 7;
	
	if (argc >= 2) {
		*threads = atoi(argv[1]);
		if (*threads <= 0) {
			printf("Invalid value for 'balls count'");
			exit(EXIT_FAILURE);
		}
	}
    
	if (argc >= 3) {
		*size = atoi(argv[2]);
		if (*size <= 0) {
			printf("Invalid value for 'throwers count'");
			exit(EXIT_FAILURE);
		}
	}
}

void msleep(UINT milisec) {
    
    time_t sec = (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    
    timespec_t req = {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    
    if(nanosleep(&req,&req)) ERR("nanosleep");
}
