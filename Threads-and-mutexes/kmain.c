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
#include <signal.h>

#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

#define DEFAULT_N 16
#define DEFAULT_T 7
#define DEFAULT_S 10
#define UNINITIALIZED -1
#define PI_CONST 3.14159265

typedef struct timespec timespec_t;
typedef unsigned int UINT;
typedef struct argsInit {
		
	pthread_t tid;
	UINT seed;
	
	int n, k;
	
	double *pVector;
	pthread_mutex_t *pmxVector; 
        
} argsInit_t;

typedef struct argsSum {
	
	pthread_t tid;
	
	int n, k;
	
	double *pVector;
	pthread_mutex_t *pmxVector;
	
} argsSum_t;

void ReadArguments(int argc, char** argv, int *threads, int *time, int *size);
void *create_inits_threads(argsInit_t *inits, int t);
void *init_work(void *inits);
void *create_sums_threads(argsSum_t *sums, int t);
void *sums_work(void *sums);

int main(int argc, char **argv)
{
	int n, t, k;
	
	ReadArguments(argc, argv, &n, &t, &k);
	
	srand(time(NULL));
	
	//Vector
	double *vector = malloc(sizeof(double) * n);
	if(vector == NULL) ERR("vector malloc");
	for(int i = 0; i < n; i++){
		vector[i] = UNINITIALIZED;
	}
	
	//Mutexes
	pthread_mutex_t *mxVector = malloc(sizeof(pthread_mutex_t) * n);
	for(int i = 0; i< n; i++){
		if(pthread_mutex_init(&mxVector[i], NULL)) ERR("Couldn't initialize mutex!");
	}
	
	//Initializer Threads
	argsInit_t *inits = malloc(sizeof(argsInit_t) * t);
	if(inits == NULL) ERR("malloc");
	
	for(int i = 0; i < t; i++){
		inits[i].seed = rand();
		inits[i].k = k;
		inits[i].n = n;
		inits[i].pVector = vector;
		inits[i].pmxVector = mxVector;
	}
	
	create_inits_threads(inits, t);
	
	void *discardInits;
	for(int i = 0; i < t; i++){
		int err = pthread_join(inits[i].tid, &discardInits);
		if(err != 0) ERR("pthread_join");
	}
	
	printf("[");
	for(int i = 0; i < n; i++){
		printf("%f ", vector[i]);
	}
	printf("]\n");
	
	int sigNo;
	sigset_t newMask, oldMask;
	sigemptyset(&newMask);
	sigaddset(&newMask, SIGINT);
	if(pthread_sigmask(SIG_BLOCK, &newMask, &oldMask)) ERR("pthread_sigmask");
	
	if(sigwait(&newMask, &sigNo));
	if(pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask)) ERR("pthread_sigmask");
	printf("\n");
	
	//SumThreads
	argsSum_t *sums = malloc(sizeof(argsSum_t) * t);
	if(sums == NULL) ERR("malloc");
	
	for(int i = 0; i < t; i++){
		sums[i].k = k;
		sums[i].n = n;
		sums[i].pVector = vector;
		sums[i].pmxVector = mxVector;
	}
	
	create_sums_threads(sums, t);
	
	double finalSum = 0.0;
	for(int i = 0; i < t; i++){
		double *partial_sum;
		int err = pthread_join(sums[i].tid, (void*)&partial_sum);
		if(err != 0) ERR("pthread_join");
		finalSum += *partial_sum;
		free(partial_sum);
	}
	
	printf("Final sum: %f\n", finalSum);
	
	free(vector);
	free(mxVector);
	free(inits);
	
	exit(EXIT_SUCCESS);
}

void ReadArguments(int argc, char** argv, int *threads, int *time, int *size) {
	
	*threads = DEFAULT_N;
	*time = DEFAULT_T;
	*size = DEFAULT_S;
	
	if (argc >= 2) {
		*threads = atoi(argv[1]);
		if (threads <= 0) {
			printf("Invalid value for 'balls count'");
			exit(EXIT_FAILURE);
		}
	}
    
	if (argc >= 3) {
		*time = atoi(argv[2]);
		if (time >= threads) {
			printf("Invalid value for 'throwers count'");
			exit(EXIT_FAILURE);
		}
	}
	
	if (argc >= 4) {
		*size = atoi(argv[3]);
		if (size <= 0 || size >= threads) {
			printf("Invalid value for 'throwers count'");
			exit(EXIT_FAILURE);
		}
	}
}

void *create_inits_threads(argsInit_t *inits, int t){
	
	for(int i = 0; i < t; i++){
		if(pthread_create(&(inits[i].tid), NULL, init_work, &inits[i])) ERR("pthread_create");
	}
}

void *init_work(void *inits){
	
	argsInit_t *init = inits;
	
	for(int i = 0; i < init->n; i++){
		pthread_mutex_lock(&init->pmxVector[i]);
		if(init->pVector[i] == UNINITIALIZED){
			init->pVector[i] = (double) rand_r(&init->seed) / (double) RAND_MAX;
		}
		pthread_mutex_unlock(&init->pmxVector[i]);
	}
	
	return NULL;
}

void *create_sums_threads(argsSum_t *sums, int t){
	
	for(int i = 0; i < t; i++){
		if(pthread_create(&(sums[i].tid), NULL, sums_work, &sums[i])) ERR("pthread_create");
	}
}

void *sums_work(void *sums){
	
	argsSum_t *sum = sums;

	double partial_res = -1;
	
	double *partial_sum = malloc(sizeof(double));
	*partial_sum = 1;
	
	for(int i = 0; i < sum->n; i++){
		bool calculated  = false;
		
		pthread_mutex_lock(&sum->pmxVector[i]);
		if(sum->pVector[i] != UNINITIALIZED){
			partial_res = sum->pVector[i];
			*partial_sum = sin(-2.0 * PI_CONST * sum->k * (partial_res/sum->n));
			sum->pVector[i] = UNINITIALIZED;
			calculated = true; 
		}
		
		pthread_mutex_unlock(&sum->pmxVector[i]);
		if(calculated) break;
	}
	
	printf("Partial sum: %f, partial result: %f\n", *partial_sum, partial_res);
	
	return partial_sum;
}
