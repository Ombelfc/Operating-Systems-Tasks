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

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ftw.h>
#include <libgen.h>

#define MAX_PATH 100
#define ERR(source) (perror(source),\
					fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
					exit(EXIT_FAILURE))

#define MAXFD 50

static unsigned long total = 0;
	
void print_cwdo(){
		
	DIR *dp;
	struct dirent *dir;
	dp = opendir(".");
	
	if(dp){
		while((dir = readdir(dp)) != NULL){
			printf("name: %s\n", dir->d_name);
		}
		closedir(dp);	
	}
}

void print_size(){
	
	DIR *dp;
	struct dirent *dir;
	struct stat filestat;
	dp = opendir(".");
	long sum = 0;
	
	if(dp){
		while((dir = readdir(dp)) != NULL){
			if(lstat(dir->d_name, &filestat)) ERR("lstat");
			sum += filestat.st_size;
			printf("File: %s, size: %ld\n", dir->d_name, filestat.st_size);
		} 
		closedir(dp);
	}
	
	printf("The sum of sizes: %ld\n", sum);
}

void print_given_directory(int num_dirs, char *dirs[]){
	
	int i;
	char path[MAX_PATH];
	
	if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");
	
	for(i = 1; i<num_dirs; i++){
		if(chdir(dirs[i])){
			 if(errno == ENOENT) continue;
			 else ERR("chdir");
		 }
		printf("Directory: %s\n", dirs[i]);
		print_size();
	}
	
	if(chdir(path)) ERR("chdir");
}

long find_directory_size(DIR *dp){

	struct dirent *dir;
	struct stat filestat;	
	long sum = 0;
	
	if(dp){
		while((dir = readdir(dp)) != NULL){
			if(lstat(dir->d_name, &filestat)) ERR("lstat");
			sum += filestat.st_size;
		}
	}
	
	return sum;
}

void add_tofolder(char *path, char *dir_name){
	
	if(chdir(path)) ERR("chdir");
	
	FILE *f;
	
	f = fopen("out.txt", "w+");
	if(f != NULL){
		fprintf(f, "Directory allowed: %s\n", dir_name);
	}
	if(fclose(f)) ERR("fclose");
	system("cat out.txt");
}

void print_directory_name1(int num_entries, char *dirs[]){
	
	int i;
	char path[MAX_PATH];
	DIR *dp;
	long sum = 0;
	long limit = 0;
	
	if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");
	
	for(i = 1; i<num_entries; i++){
		limit = atoi(dirs[i+1]);
		if(chdir(dirs[i])){
			if(errno == ENOENT) continue;
			else ERR("chdir");
		}
		dp = opendir(".");
		sum = find_directory_size(dp);
		if(sum > limit) {
			printf("Folder sum: %ld, threshold: %ld; 'Directory associated: %s'\n", sum, limit, dirs[i]);
			add_tofolder(path, dirs[i]);
		}
		closedir(dp);
		sum = 0;
		i++;
	}
	
	if(chdir(path)) ERR("chdir");
}

int sum(const char *dir, const struct stat *filestat, int typeflag, struct FTW *f){
	
	total += filestat->st_size;
	return 0;
}

void print_directory_name2(int nums, char *dirs[]){
	
	int i;
	long limit = 0;
	
	for(i = 1; i<nums; i++){
		limit = atoi(dirs[i+1]);
		if(nftw(dirs[i], sum, MAXFD, FTW_PHYS) == 0){
			if(total > limit) printf("Folder sum: %ld, threshold: %ld; 'Directory associated: %s'\n", total, limit, dirs[i]);
		}
		else printf("%s: access denied\n", dirs[i]);
		total = 0;
		i++;
	} 
}

int walk(const char *path, const struct stat *filestat, int typeflag, struct FTW *f){

	printf("Object: [%s], size: [%ld]\n", path, filestat->st_size);
	
	return 0;
}

void print_directory_recursively(int nums, char *dirs[]){

	int i;
	
	for(i = 0; i<nums; i++){
		printf("Directory being walked ... \n");
		if(nftw(dirs[i], walk, MAXFD, FTW_PHYS) == 0){
			printf("Directory walked recursively/exhansted\n");
		}
		else if(errno == ENOENT) continue;
		else printf("%s: access denied\n", dirs[i]);
	}
}

int walk_s(const char *name, const struct stat *s, int type, struct FTW *f){
	
	const char *olddir = getenv("PWD");
	//printf("PWD is: %s\n", olddir);
	const char *l1_lindir = getenv("L1_LNDIR");
	//printf("L1_LNDR is: %s\n", l1_lindir);
	
	switch(type){
		case FTW_F:
			if(s->st_mode & S_IXUSR | S_IXGRP | S_IXOTH){
				printf("File: [%s]\n", name);
				char *bname = basename(name);
				char *dname = dirname(name);
				
				chdir(l1_lindir);
				//printf("basename is: %s\n", bname);
				//printf("dirname is: %s\n", dname);
				
				symlink(name, bname);
				chdir(dname);
				printf("[%s] file, [%s] [%s] link\n", name, olddir, bname);
			}
			break;
	}
	
	return 0;
}

void create_symlinks_cond(int argc, char *argv[]){
	
	int i;
	char c;
	
	while((c = getopt(argc, argv, "t:")) != 1){
		switch(c){
			case 't':
				chdir(optarg);
				nftw(".", walk_s, MAXFD, FTW_PHYS);
				break;
		}
	}
}

void Usage(char *name){
	
	printf("Usage: %s, directory1_name, limit1, directory2_name, limit2, .....\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	//if(argc % 2 == 0) Usage(argv[0]);
	
	//Print names of objects in cwd 01
	//system("ls");
	
	//Print names of objects in cwd 02
	//print_cwdo();
	
	//Print size of objects in cwd
	//print_size();
	
	//Print sizes and names of objects of the given directory(s)
	//print_given_directory(argc, argv);
	
	//Print name of folder if content size sum is above limit 01
	//print_directory_name1(argc, argv);
	
	//Print name of folder if content size sum is above threshold (including subfolders) 02
	//print_directory_name2(argc, argv);
	
	//Print names of the given objects recursively
	//print_directory_recursively(argc, argv);
	
	//Lab01 solution
	create_symlinks_cond(argc, argv);
	
	return EXIT_SUCCESS;
}
