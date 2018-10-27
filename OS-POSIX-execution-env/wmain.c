/*
 * wmain.c
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>

#define MAXFD 50

#define ERR(source) (perror(source),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))

static unsigned int dirrs = 0, files = 0, max = 0, min = -1;
static unsigned int size = 0;

char biggest[100] = "";
char smallest[100] = "";

struct arg_tuple {
	
	int a_option;
	char filename[256];
	
} tuple;

void Usage(char *name);

void get_arguments(int argc, char *argv[]){
	
	char c;
	while((c = getopt(argc, argv, "a::o:s:")) != -1){
		switch(c){
			case 's':
				size = strtol(optarg, (char **)NULL, 10);
				break;
			case 'o':
				strcpy(tuple.filename, optarg);
				break;
			case 'a':
				tuple.a_option = 1;
				break;
			case '?':
			default:
				Usage(argv[0]);
				break;
		}
	}
}

int walk(const char *path, const struct stat *filestat, int typeflag, struct FTW *f){
	
	if(min == -1) min = filestat->st_size;
	
	switch(typeflag){
		case FTW_D:
			dirrs++;
			break;
		case FTW_F:
			if(filestat->st_size < size) break;
			files++;
			if(filestat->st_size > max){
				max = filestat->st_size;
				strcpy(biggest, path);
			}
			else if(filestat->st_size < min){
				min = filestat->st_size;
				strcpy(smallest, path);
			}
			break;
		default:
			break;
	}
	return 0;
}

void print_dirs(int num_dirs, char *dirs[]){
	
	int i;
	
	for(i = 1; i<num_dirs; i++){
		if(nftw(dirs[i], walk, MAXFD, FTW_PHYS) == 0){
			printf("Directory %s being walked. Results:\n", dirs[i]);
			printf(" Biggest: %s, %d \n Smallest: %s, %d \n directories: %d \n files: %d\n", biggest, max, smallest, min, dirrs, files);
		}
		else{
			printf("Can't Access the directory\n");
			continue;
		}
	}
}

void write_to_file(){
	
	FILE *f;
	
	char *mode = tuple.a_option == 0 ? "w+" : "a+";
	f = fopen(tuple.filename, mode);
	
	if(f != NULL){
		fprintf(f, "Biggest: %s, %d \n Smallest: %s, %d \n directories: %d \n files: %d\n", biggest, max, smallest, min, dirrs, files);
	}
	
	if(fclose(f)) ERR("fclose");
}

void Usage(char *name){
	
	printf("USAGE: %s -a -o filename -s size /path \n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{	
	//if(argc != 6 || argc != 7) Usage(argv[0]);
	
	get_arguments(argc, argv);
	
	print_dirs(argc, argv);
	
	write_to_file();
	
	return EXIT_SUCCESS;
}

