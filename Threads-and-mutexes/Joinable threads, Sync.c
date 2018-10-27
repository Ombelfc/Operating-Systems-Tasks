/*
 * Main.c
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
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#define MAXLINE 4096
#define DEFAULT_THREADCOUNT 10
#define DEFAULT_SAMPLESIZE 100

#define ERR(source) (perror(source),\
					 fprintf(stderr, "%s:%d\n", __FILE__,__LINE__),\
					 exit(EXIT_FAILURE))
					 
typedef unsigned int UINT;

typedef struct argsEstimation {
	
	pthread_t tid;
	UINT seed;
	int samplesCount;
	
} argsEstimation_t;

void ReadArguments(int argc, char **argv, int *threadCount, int *samplesCount);
void *pi_estimation(void *args);

int main(int argc, char **argv){
	
	int threadCount, samplesCount;
	double *subresult;
	
	ReadArguments(argc, argv, &threadCount, &samplesCount);
	
	argsEstimation_t *estimations = (argsEstimation_t*) malloc(sizeof(argsEstimation_t) *threadCount);
	if(estimations == NULL) ERR("Malloc error for estimation arguments!");
	
	srand(time(NULL));
	for(int i = 0; i<threadCount; i++){
		estimations[i].seed = rand();
		estimations[i].samplesCount = samplesCount;
	}
	
	for(int i = 0; i<threadCount; i++){
		int err = pthread_create(&(estimations[i].tid), NULL, pi_estimation, &estimations[i]);
		if(err != 0) ERR("Couldn't create thread");
	}
	
	double cumulativeResult = 0.0;
	for(int i = 0; i<threadCount; i++){
		int err = pthread_join(estimations[i].tid, (void*)&subresult);
		if(err != 0) ERR("Can't join with a thread");
		if(NULL != subresult){
			cumulativeResult += *subresult;
			free(subresult);
		}
	}
	
	double result = cumulativeResult / threadCount;
	printf("PI ~= %f\n", result);
	free(estimations);
	
	return EXIT_SUCCESS;
}					 

void ReadArguments(int argc, char **argv, int *threadCount, int *samplesCount) {
	
	*threadCount = DEFAULT_THREADCOUNT;
	*samplesCount = DEFAULT_SAMPLESIZE;
	
	if(argc >= 2){
		*threadCount = atoi(argv[1]);
		if(*threadCount <= 0) {
			printf("Invalid value for 'threadCount'");
			exit(EXIT_FAILURE);
		}
	}
	
	if(argc >= 3){
		*samplesCount = atoi(argv[2]);
		if(*samplesCount <= 0){
			printf("Invalid value for 'samplesCount'");
			exit(EXIT_FAILURE);
		}
	}
}

void *pi_estimation(void *voidPtr){
	
	argsEstimation_t *args = voidPtr;
	double *result;
	
	if(NULL == (result = malloc(sizeof(double)))) ERR("malloc:");
	
	int insideCount = 0;
	for(int i = 0; i<args->samplesCount; i++){
		double x = ((double) rand_r(&args->seed) / (double) RAND_MAX);
		double y = ((double) rand_r(&args->seed) / (double) RAND_MAX);
		if(sqrt(x*x+y*y) <= 1.0) insideCount++;
	}
	
	*result = 4.0*(double) insideCount / (double) args->samplesCount;
	
	return result;
}
