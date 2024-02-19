#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>



struct PCB{
	int PID; //process id
	int START; //start position in shell memory
	int SCORE;
	int PROG;
	struct PCB *NEXT;
	char *CMD;
	int PAGENO;
	int LINE;
};

int interpreter(char* command_args[], int args_size);
int help();
int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script, int i);
int echo(char* var);
int my_ls();
int my_mkdir(char* dirname);
int my_touch(char* filename);
int my_cd(char* dirname);
void execute();
char* intToString(int value);
int exec(char* command[], int size);
bool checkDup(char* command_args[], int size);
void rearrange_by_line_count(char* files[], int arg_count);
int get_file_line_count(char* filename);
void add_queue(int pid, int start, int score, int prog, char arr[], int pageno, int line);
int add_file(char* script, int prog);
void editReadyQueue(int current_prog);
int aging(char *command_args[], int size);
void printReadyQueue();
int findNearestSmallest();
void move_front(int n);
void move_back(int n);
int RR(char *filename_array[], int num_files, int time_slice);
int del_dir(const char *dir_name);
void copyFile(char *input_filename);
int backStore_files(const char* dirname, char* file_names[], int num_files);
int make_pages(char *filename_array[], int num_files, int time_slice);
void print_page_table_array();
int add_to_memory();
void undo_dequeue(struct PCB *pcb);
void add_pageno(int program, int pageno);
void durarara();
void move_back_two(int n);
int check(char *command_args[], int args_size);
int check_helper(char *command_args[], int args_size);
void res(int i);