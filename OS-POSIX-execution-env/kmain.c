/*
 * L1ListDirs.c
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

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ftw.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#define ERR(source) (perror(source),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))

#define MAX_PATH 256
#define MAXFD 50

struct args {
	
	int ap_option;
	char filename[MAX_PATH];
} arg;

static unsigned int size = 0;
FILE *output;

void Usage(char *name);

int walk(const char *path, const struct stat *filestat, int typeflag, struct FTW *f){
	
	if(filestat->st_size > size){
		fprintf(output, "Object: [%s], size: [%ld]\n", path, filestat->st_size);
	}
	return 0;
}

void print_dirs(int num_dirs, char *dirs[]){
	
	int i;
	
	/*Printing inputed directories*/
	for(i = 1; i<num_dirs; i++){
		printf("Directory being walked %s\n", dirs[i]);
		if(nftw(dirs[i], walk, MAXFD, FTW_PHYS) == 0){
			printf("Directory walked/exhausted.\n");
		}
		else if(errno == ENOENT) continue;
		else ERR("nftw");
	}
}

void get_params(int argc, char *argv[]){
		
	char c;
	while((c = getopt(argc, argv, "s:a::f:")) != -1){
		switch(c){
			case 's':
				size = atoi(optarg);
				break;
			case 'a':
				arg.ap_option = 1;
				break;
			case 'f':
				output = fopen(optarg, arg.ap_option == 0 ? "w+" : "a+");
				strcpy(arg.filename, optarg);
				break;
			case '?':
			default :
				Usage(argv[0]);
				break;
		}
	}
}

int walk_cwd(const char *path, const struct stat *filestat, int typeflag, struct FTW *f){
	
	if(filestat->st_size > size){
		fprintf(output, "Object: [%s], size: [%ld]\n", path, filestat->st_size);
	}
	return 0;
}

void print_cwd(){
		
	/*Getting the current working directory*/
	char path[MAX_PATH];
	if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");
	
	printf("Current working directory being walked ... \n");
	if(nftw(path, walk_cwd, MAXFD, FTW_PHYS) == 0){
		printf("Directory walked/exhausted.\n");
	}
	else ERR("nftw");	
}

void Usage(char *name){
	
	fprintf(stderr, "USAGE: %s /dir1 .... /dirN -s size -a -f output.txt\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	output = stdout;
	
	//get append option and filename
	get_params(argc, argv);
	
	if(argc < 2) print_cwd();
	
	print_dirs(argc, argv);
	
	return EXIT_SUCCESS;
}

