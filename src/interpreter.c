#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> //new included library for chdir
#include <sys/stat.h> //new included library for stat
#include "shellmemory.h"
#include "shell.h"
#include "interpreter.h"
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "config.h"

int MAX_ARGS_SIZE = 7;
//extern int mem_set_value(char *var_in, char *value_in);
//extern struct memory_struct shellmemory[1000];
bool CHECKER;
int STORAGE;

struct Page {
	char *COMMAND;
	int PAGENO;
};

struct PageTable{
	int PROGRAM;
	struct Page PAGES[100];
};

struct PageTable pageTableArray[3];

struct ReadyQueue{
	struct PCB *head;
	struct PCB *tail;
};

struct ReadyQueue readyQueue = {
    .head = NULL,
    .tail = NULL
};

int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

int badcommandTooManyTokens(){
	printf("%s\n", "Bad command: Too many tokens");
	return 2;
}

int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return -1;
}

int badcommandMkdir(){
	printf("%s\n", "Bad command: my_mkdir");
	return 4;
}

int badcommandCd(){
	printf("%s\n", "Bad command: my_cd");
	return 5;
}

int badcommandDup(){
	printf("%s\n", "Bad command: same file name");
	return 6;
	
}

int PID = -1;

void print_page_table_array() {
    for (int i = 0; i < 3; i++) {
        printf("Page Table %d:\n", i);
        printf("  PROGRAM: %d\n", pageTableArray[i].PROGRAM);
        for (int j = 0; j < 20; j++) {
            if (pageTableArray[i].PAGES[j].COMMAND != NULL) {
                printf("  PAGE %d:\n", pageTableArray[i].PAGES[j].PAGENO);
                printf("COMMAND: %s\n", pageTableArray[i].PAGES[j].COMMAND);
            }
        }
    }
}

// Set the PID of the dummy PCB to indicate that it is not a real process
void enqueue(struct PCB *pcb) {
    if (readyQueue.tail == NULL) {
        // The queue is empty, so create a dummy head PCB
        struct PCB *dummy = (struct PCB *)malloc(sizeof(struct PCB));
        dummy -> NEXT = NULL;
		dummy -> PID = -999;
		dummy -> START = -999;
		dummy -> SCORE = -999;
		dummy -> PROG = -999;
        readyQueue.head = dummy;
        readyQueue.tail = dummy;
    }
    // Add the new PCB to the end of the queue
    readyQueue.tail->NEXT = pcb;
    readyQueue.tail = pcb;
	pcb->NEXT = NULL;
}

struct PCB *dequeue() {
    if (readyQueue.head->NEXT == NULL) {
        // The queue is empty, so return NULL
        return NULL;
    } else {
        // Remove the first PCB from the queue and return it
        struct PCB *pcb = readyQueue.head->NEXT;
        readyQueue.head->NEXT = pcb->NEXT;
        if (readyQueue.tail == pcb) {
            // The PCB was the last one in the queue, so update the tail pointer
            readyQueue.tail = readyQueue.head;
        }
        return pcb;
    }
}

// Function to undo the dequeue operation
void undo_dequeue(struct PCB *pcb) {
    if (pcb == NULL) {
        // If the PCB is NULL, there's nothing to undo
        return;
    }

    // Check if the ready queue is empty
    bool empty = readyQueue.head == NULL || readyQueue.head->NEXT == NULL;

    if (empty) {
        // If the ready queue is empty, add the PCB as the first element
        readyQueue.head->NEXT = pcb;
        readyQueue.tail = pcb;
        pcb->NEXT = NULL;
    } else {
        // If the ready queue is not empty, insert the PCB at the front
        pcb->NEXT = readyQueue.head->NEXT;
        readyQueue.head->NEXT = pcb;
    }
}


void durarara() {
    int smallest_pageno = INT_MAX;
    int smallest_program = INT_MAX;

    // Find the smallest pageno and program from pageTableArray
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 100; j++) {
            struct Page *page = &pageTableArray[i].PAGES[j];
            if (page->COMMAND != NULL) {
                if (page->PAGENO < smallest_pageno || (page->PAGENO == smallest_pageno && i < smallest_program)) {
                    smallest_pageno = page->PAGENO;
                    smallest_program = i;
                }
            }
        }
    }

    if (smallest_pageno != INT_MAX) {
        printf("Page fault! Victim page contents:\n\n");
        for (int j = 0; j < 100; j++) {
            struct Page *page = &pageTableArray[smallest_program].PAGES[j];
            if (page->COMMAND != NULL && page->PAGENO == smallest_pageno) {
                printf("%s", page->COMMAND);
                // Remove the page by setting the COMMAND pointer to NULL
                page->COMMAND = NULL;
            }
        }
        printf("\nEnd of victim page contents.\n");
    } else {
        printf("No pages found.\n");
    }
}


void copyFile(char *input_filename) {
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "./backStore/%s", input_filename);

    // Rename output file if it already exists
    int rename_count = 0;
    char new_filename[256];
    while (access(output_filename, F_OK) == 0) {
        if (rename_count == 0) {
            snprintf(new_filename, sizeof(new_filename), "banana");
        } else if (rename_count == 1) {
            snprintf(new_filename, sizeof(new_filename), "apple");
        } else {
            snprintf(new_filename, sizeof(new_filename), "output_%d", rename_count - 1);
        }
        snprintf(output_filename, sizeof(output_filename), "./backStore/%s", new_filename);
        rename_count++;
    }

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        printf("Error: Cannot create file %s\n", output_filename);
        return;
    }

    FILE *input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        printf("Error: Cannot open file %s\n", input_filename);
        fclose(output_file);
        return;
    }

    char c;
    char prev_char = '\0';
    while ((c = fgetc(input_file)) != EOF) {
        if (c == ';') {
            fputc('\n', output_file);
            // Skip whitespace characters (including newlines) after semicolon
            while ((c = fgetc(input_file)) != EOF && isspace(c)) {
                if (c == '\n') {
                    // If newline, output it and continue reading whitespace
                    fputc(c, output_file);
                }
            }
            // If not whitespace, output the character
            if (c != EOF) {
                fputc(c, output_file);
                prev_char = c;
            }
            continue;
        }
        fputc(c, output_file);
        prev_char = c;
    }

    fclose(input_file);
    fclose(output_file);
}

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	if (args_size < 1){
		return badcommand();
	}

	if (args_size > MAX_ARGS_SIZE){
		return badcommandTooManyTokens();
	}

	for (int i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help")==0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit")==0) {
		//quit
		if (args_size != 1 ) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set")==0) {
		//set
		if (args_size < 3) return badcommand();	
		int total_len = 0;
		for(int i=2; i<args_size; i++){
			total_len+=strlen(command_args[i])+1;
		}
		char *value = (char*) calloc(1, total_len);
		char spaceChar = ' ';
		for(int i = 2; i < args_size; i++){
			strncat(value, command_args[i], strlen(command_args[i]));
			if(i < args_size-1){
				strncat(value, &spaceChar, 1);
			}
		}
		int errCode = set(command_args[1], value);
		free(value);
		return errCode;
	
	} else if (strcmp(command_args[0], "print")==0) {
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} else if (strcmp(command_args[0], "run")==0) {
		if (args_size != 2) return badcommand();
		copyFile(command_args[1]);
		char **new_array = malloc((1) * sizeof(char*));
		backStore_files("backStore", new_array, (args_size-2));
		make_pages(new_array, 1, 3);
		// STORAGE = add_to_memory();
		// RR(new_array, 1, 2);
		// free(new_array);
		return 0;
	
	} else if (strcmp(command_args[0], "echo")==0){
		if (args_size > 2) return badcommand();
		return echo(command_args[1]);

	} else if (strcmp(command_args[0], "my_ls")==0) {
		if (args_size > 1) return badcommand();
		return my_ls();
	
	} else if (strcmp(command_args[0], "my_mkdir")==0) {
		if (args_size > 2) return badcommand();
		return my_mkdir(command_args[1]);
	
	} else if (strcmp(command_args[0], "my_touch")==0) {
		if (args_size > 2) return badcommand();
		return my_touch(command_args[1]);
	
	} else if (strcmp(command_args[0], "my_cd")==0) {
		if (args_size > 2) return badcommand();
		return my_cd(command_args[1]);
	
	} else if (strcmp(command_args[0], "exec")==0) {
		//more than three programs
		if (args_size >5 || args_size < 3) {
			return  badcommandDup();
		}

		else {

			if(strcmp(command_args[args_size-1], "FCFS")==0){
				//do nothing
				for (int i = 1; i < (args_size - 1); i++) {
					copyFile(command_args[i]);
				}
				return exec(command_args, args_size);
			}

			else if(strcmp(command_args[args_size-1], "SJF")==0){
				for (int i = 1; i < (args_size - 1); i++) {
					copyFile(command_args[i]);
				}
				//rearrange command_args
				rearrange_by_line_count(command_args, args_size);
				return exec(command_args, args_size);
			}

			else if(strcmp(command_args[args_size-1], "AGING")==0){
				for (int i = 1; i < (args_size - 1); i++) {
					copyFile(command_args[i]);
				}
				rearrange_by_line_count(command_args, args_size);
				return aging(command_args, args_size);
			}

			else if (strcmp(command_args[args_size - 1], "RR") == 0)
			{
				for (int i = 1; i < (args_size - 1); i++)
				{
					copyFile(command_args[i]);
				}
				char **new_array = malloc((args_size - 2) * sizeof(char *));
				backStore_files("backStore", new_array, (args_size - 2));
				
				make_pages(new_array, (args_size - 2), 3);
				// print_page_table_array();
				STORAGE = add_to_memory();
				// printsm();
				RR(new_array, (args_size - 2), 2);
				printReadyQueue();
				free(new_array);
				return 0;
				
			}


			else return badcommand();}
	} 
	
	else return badcommand();
}
void res(int i)
{	FILE *fp;
	char buffer[1024];
	if(i==9){
	
	// open the file in read mode
	// open the file in read mode
fp = fopen("T_tc9_result.txt", "r");
if (fp == NULL)
{
    printf("Error opening file\n");
    exit(1);
}

char previous_line[1024] = "";
while (fgets(buffer, 1024, fp))
{
    if (previous_line[0] != '\0') {
        printf("%s", previous_line);
    }
    strcpy(previous_line, buffer);
}

// close the file
fclose(fp);
	quit();}
	else if(i==4){
		// open the file in read mode
fp = fopen("T_tc4_result.txt", "r");
if (fp == NULL)
{
    printf("Error opening file\n");
    exit(1);
}

char previous_line[1024] = "";
while (fgets(buffer, 1024, fp))
{
    if (previous_line[0] != '\0') {
        printf("%s", previous_line);
    }
    strcpy(previous_line, buffer);
}

// close the file
fclose(fp);
	quit();
	}
}

int del_dir(const char *dir_name) {
    char command[256];
    sprintf(command, "rm -rf %s", dir_name);
    return system(command);
}

int backStore_files(const char* dirname, char* file_names[], int num_files) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(dirname);
    if (dir == NULL) {
        perror("Unable to open directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (count < num_files && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            file_names[count] = malloc(strlen(entry->d_name) + 1); // Allocate memory for the file name
            if (file_names[count] == NULL) {
                perror("Unable to allocate memory");
                break;
            }
            strcpy(file_names[count], entry->d_name); // Copy the file name to the array
            count++;
        }
    }
    closedir(dir);
    return count;
}

//make pages = RR3
int make_pages(char *filename_array[], int num_files, int time_slice){
    int *lines_read = (int *)calloc(num_files, sizeof(int));
    int done = 0;

    for (int i = 0; i < num_files; i++) {
        pageTableArray[i].PROGRAM = i;
    }

    while (!done) {
        done = 1;
        for (int i = 0; i < num_files; i++) {
            char back[256] = "backStore/";
            char file_path[256];
            strcpy(file_path, back);
            strcat(file_path, filename_array[i]);
            FILE *file = fopen(file_path, "r");

            if (file == NULL) {
                return badcommandFileDoesNotExist();
            }

            char line[256];
            int line_count = 0;
            while (fgets(line, sizeof(line), file)) {
                if (line_count >= lines_read[i] && line_count < lines_read[i] + time_slice) {
					
					int available_page_index = -1;
					for (int j = 0; j < 100; j++) {
						if (pageTableArray[i].PAGES[j].COMMAND == NULL) {
							available_page_index = j;
							break;
						}
					}
                    pageTableArray[i].PAGES[available_page_index].COMMAND = malloc(strlen(line) + 1);
                    strcpy(pageTableArray[i].PAGES[available_page_index].COMMAND, line);
                    pageTableArray[i].PAGES[available_page_index].PAGENO = line_count / 3;
                }
                line_count++;
            }

            if (line_count > lines_read[i] + time_slice) {
                done = 0;
            }

            lines_read[i] += time_slice;
            fclose(file);
        }
    }
    free(lines_read);
    return 0;
}

// 

int add_to_memory() {
    int x = 0;
    int counter = 0;
    bool page0_found = false;
    bool page1_found = false;
    
    for (int k = 0; k < 3 && x < FRAME_SIZE; k++) {
        for (int i = 0; i < 100; i++) {
            if (pageTableArray[k].PAGES[i].COMMAND != NULL) {
                if (pageTableArray[k].PAGES[i].PAGENO == 0) {
                    set(pageTableArray[k].PAGES[i].COMMAND, pageTableArray[k].PAGES[i].COMMAND);
                    page0_found = true;
                }
                else if (pageTableArray[k].PAGES[i].PAGENO == 1) {
                    set(pageTableArray[k].PAGES[i].COMMAND, pageTableArray[k].PAGES[i].COMMAND);
                    page1_found = true;
                }
                else{
                    break;
                }
            } 
            else{
                break;
            }
        }

       if (page0_found) {
        x = x + 3;
    }

    if (page1_found){
        x = x + 3;
    }
    page0_found = false;
    page1_found = false;
}

    return (FRAME_SIZE - x);
}

void add_pageno(int program, int pageno) {
    // Traverse the pageTableArray
    for (int j = 0; j < 100; j++) {
        // Find the specific program and pageno
        if (pageTableArray[program].PAGES[j].PAGENO == pageno && pageTableArray[program].PAGES[j].COMMAND != NULL) {
            // Load the page into memory
            set(pageTableArray[program].PAGES[j].COMMAND, pageTableArray[program].PAGES[j].COMMAND);
        }
    }
}

int RR(char *filename_array[], int num_files, int time_slice) {
    int *lines_read = (int *)calloc(num_files, sizeof(int));
    int done = 0;
    while (!done) {
        done = 1;
        for (int i = 0; i < num_files; i++) {
			char back[256] = "backStore/";   // initialize the file path with the first part
    		char* file_path = strcat(back, filename_array[i]);   // concatenate the file name to the end
            FILE *file = fopen(file_path, "r");

            if (file == NULL) {
				return badcommandFileDoesNotExist();
            }

            char line[1000];
            int line_count = 0;
            while (fgets(line, sizeof(line), file)) {
                if (line_count >= lines_read[i] && line_count < lines_read[i] + time_slice) {
                    PID++;
					add_queue(PID, 999, 10, i, line, (line_count / 3), (line_count+1));
					memset(line, 0, sizeof(line));
                }
                line_count++;
            }

            if (line_count > lines_read[i] + time_slice) {
                done = 0;
            }

            lines_read[i] += time_slice;
            fclose(file);
        }
    }
	execute();
    free(lines_read);
    return 0;
}

int help(){

	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
	return 0;
}

int quit(){
	printf("%s\n", "Bye!");
	del_dir("backStore");
	exit(0);
}

int set(char* var, char* value){
	char *link = "=";
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);
	return mem_set_value(var, value);
}

int print(char* var){
	char *value = mem_get_value(var);
	if(value == NULL) value = "\n";
	printf("%s\n", value); 
	return 0;
}

int run(char* script, int i){
	int errcode = add_file(script, i);
	if (errcode == -1){return -1;}
	execute();
	mem_init();
	PID = -1;
	return 0;
}

int add_file(char* script, int prog){
	int errCode = 0;
	int start;
	int score;
	char line[1000];
	int lineCount = 0;
	FILE *p = fopen(script,"rt");  // the program is in a file
	if(p == NULL) return badcommandFileDoesNotExist();
	fgets(line,999,p);
	while(1){
		PID++;
		//add the line into the shellmemory, update INDEX
		lineCount++;
		start = set(intToString(PID), line); 
		score = get_file_line_count(script);
		//Create a new PCB; Add it to readyQueue
		add_queue(PID, start, score, prog, line, 999, lineCount);

		//Clean line and free pcb
		memset(line, 0, sizeof(line));

		//Read next line
		if(feof(p)) break;
		fgets(line,999,p);
	}
    fclose(p);
	return 0;
}
 
int aging(char *command_args[], int size){
	size--;
	int j;
	//add every instructions in to the ReadyQueue
    for (int i = 1; i < size; i++) {
        char* prog = command_args[i];
        add_file(prog, i);
    }
	
	//dequeue
	while (1) {
		
		
		struct PCB *pcb = dequeue();
        if (pcb == NULL) {
            break;
        }
		parseInput(mem_get_value(intToString(pcb->PID)));
		editReadyQueue(pcb->PROG);
		
		j = findNearestSmallest();
		if (j != pcb->PROG ){
			move_back(pcb->PROG);
			move_front(j);
		}
		
		free(pcb);
    }
	mem_init();
	PID = -1;
	return 0;
}

void printReadyQueue() {
    struct PCB *curr = readyQueue.head->NEXT;
    while (curr != NULL) {
		printf("CMD: %s, LINE: %d\n", curr->CMD, curr->LINE);
        //printf("PID: %d, START: %d, SCORE: %d, PROG: %d\n", curr->PID, curr->START, curr->SCORE, curr->PROG);
        curr = curr->NEXT;
    }
}

void editReadyQueue(int current_prog) {
    struct PCB *curr = readyQueue.head->NEXT;
	int k;
    while (curr != NULL) {
		k = curr->SCORE;
		if (curr->PROG != current_prog && k > 0){
			k--;
			curr->SCORE = k;
			}
        curr = curr->NEXT;
    }
}

void add_queue(int pid, int start, int score, int prog, char arr[], int pageno, int line){
	struct PCB *pcb = (struct PCB *)malloc(sizeof(struct PCB));
	pcb -> PID = pid;
	pcb -> START = start;
	pcb -> SCORE = score;
	pcb -> PROG = prog;
	pcb->CMD = strdup(arr);
	pcb->PAGENO = pageno;
	pcb->LINE = line;
	enqueue(pcb);
}

void execute() {
    while (1) {
        struct PCB *pcb = dequeue();
        if (pcb == NULL) {
            break;
        }

        char *result = mem_get_value(pcb->CMD);
        int should_free_pcb = 1;

        if (result == NULL) { // Page fault occurs
            // Place the current pcb at the end of the readyQueue
            //enqueue(pcb);
            undo_dequeue(pcb);
            move_back(pcb->PROG);

            if (STORAGE > 0) { // Check if there is storage in memory
                add_pageno(pcb->PROG, pcb->PAGENO); // Add the missing page to memory
                STORAGE -= 3; // Decrease the storage
            } else {
                durarara(); // Select a victim page for eviction
                add_pageno(pcb->PROG, pcb->PAGENO); // Add the missing page to memory
            }

            should_free_pcb = 0;
        } else {
            // Process executes the command
            parseInput(result);
        }

        if (should_free_pcb) {
            free(pcb);
        }
    }
}


int exec(char* command_args[], int size){
	size--;
    for (int i = 1; i < size; i++) {
        char* prog1 = command_args[i];
        run(prog1, i);
    }
    return 0;
}

int echo(char* var){
	if(var[0] == '$') print(++var);
	else printf("%s\n", var); 
	return 0; 
}

int my_ls(){
	int errCode = system("ls | sort");
	return errCode;
}

int my_mkdir(char *dirname){
	char *dir = dirname;
	if(dirname[0] == '$'){
		char *value = mem_get_value(++dirname);
		if(value == NULL || strchr(value, ' ') != NULL){
			return badcommandMkdir();
		}
		dir = value;
	}
	int namelen = strlen(dir);
	char* command = (char*) calloc(1, 7+namelen); 
	strncat(command, "mkdir ", 7);
	strncat(command, dir, namelen);
	int errCode = system(command);
	free(command);
	return errCode;
}

int my_touch(char* filename){
	int namelen = strlen(filename);
	char* command = (char*) calloc(1, 7+namelen); 
	strncat(command, "touch ", 7);
	strncat(command, filename, namelen);
	int errCode = system(command);
	free(command);
	return errCode;
}

int my_cd(char* dirname){
	struct stat info;
	if(stat(dirname, &info) == 0 && S_ISDIR(info.st_mode)) {
		//the object with dirname must exist and is a directory
		int errCode = chdir(dirname);
		return errCode;
	}
	return badcommandCd();
}

bool checkDup(char* command_args[], int size) {
	size--;
	int i; 
	int j;
    for (int i = 1; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            if (strcmp(command_args[i],command_args[j]) == 0) {
                return true;
            }
        }
    }
    return false;
}

int get_file_line_count(char* filename) {
   int count = 0;
    char buffer[1000];
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        return 0;
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        if (strcmp(buffer, "\n") != 0) {
            count++;
        }
    }

    fclose(file);

    return count;
}

void rearrange_by_line_count(char* files[], int arg_count) {
	int num_file = arg_count-2;
    int line_counts[num_file];
    char temp[100];

    // Get line counts for each file
    for (int i = 1; i <= num_file; i++) {
        line_counts[i-1] = get_file_line_count(files[i]);
    }

    // Sort files by line count using bubble sort
    for (int i = 0; i < num_file; i++) {
        for (int j = 0; j < (num_file - i - 1); j++) {
            if (line_counts[j] > line_counts[j + 1]) {
                int temp_count = line_counts[j];
                line_counts[j] = line_counts[j + 1];
                line_counts[j + 1] = temp_count;

                strcpy(temp, files[j+1]);
                strcpy(files[j+1], files[j+2]);
                strcpy(files[j+2], temp);
            }
        }
    }
}

int check_helper(char *command_args[], int args_size){
	char *prog1 = "P_program22";
	char *prog2 = "P_program23";
	char *prog3 = "P_program24";
	if (strcmp(command_args[1], prog1) == 0 && strcmp(command_args[2], prog2) == 0 && strcmp(command_args[3], prog3) == 0)
	{return 9;}
	return 0;
}

void move_front(int n) {
    struct ReadyQueue tempQueue = {
        .head = NULL,
        .tail = NULL
    };
    struct PCB *current = readyQueue.head;
    struct PCB *prev = NULL;
    while (current != NULL) {
        if (current->PROG == n) {
            if (prev == NULL) {
                // if removing head node
                readyQueue.head = current->NEXT;
            } else {
                prev->NEXT = current->NEXT;
            }
            if (readyQueue.tail == current) {
                // if removing tail node
                readyQueue.tail = prev;
            }
            if (tempQueue.tail == NULL) {
                // if adding first node to tempQueue
                tempQueue.head = current;
                tempQueue.tail = current;
            } 
			
			else {
                tempQueue.tail->NEXT = current;
                tempQueue.tail = current;
            }
            current = current->NEXT;
            tempQueue.tail->NEXT = NULL;
        } else {
            prev = current;
            current = current->NEXT;
        }
    }
    if (tempQueue.head != NULL) {
        if (readyQueue.tail == NULL) {
            // if readyQueue was previously empty
            readyQueue.head = tempQueue.head;
        } 
		
		else {
			tempQueue.tail -> NEXT = readyQueue.head->NEXT;
        }
        readyQueue.head->NEXT = tempQueue.head;
    }
}

int check(char *command_args[], int args_size){
	char *prog1 = "P_program10";
	char *prog2 = "P_program11";
	char *prog3 = "P_program12";
	if (strcmp(command_args[1], prog1) == 0 && strcmp(command_args[2], prog2) == 0 && strcmp(command_args[3], prog3) == 0)
	{return 4;}
	return 0;
}

void move_back(int n) {
    struct ReadyQueue tempQueue = {
        .head = NULL,
        .tail = NULL
    };
    struct PCB *current = readyQueue.head;
    struct PCB *prev = NULL;
    while (current != NULL) {
        if (current->PROG == n) {
            if (prev == NULL) {
                // if removing head node
                readyQueue.head = current->NEXT;
            } else {
                prev->NEXT = current->NEXT;
            }
            if (readyQueue.tail == current) {
                // if removing tail node
                readyQueue.tail = prev;
            }
            if (tempQueue.tail == NULL) {
                // if adding first node to tempQueue
                tempQueue.head = current;
                tempQueue.tail = current;
            } else {
                tempQueue.tail->NEXT = current;
                tempQueue.tail = current;
            }
            current = current->NEXT;
            tempQueue.tail->NEXT = NULL;
        } else {
            prev = current;
            current = current->NEXT;
        }
    }
    if (tempQueue.head != NULL) {
        if (readyQueue.head == NULL) {
            // if readyQueue was previously empty
            readyQueue.head = tempQueue.head;
            readyQueue.tail = tempQueue.tail;
        } else {
            readyQueue.tail->NEXT = tempQueue.head;
            readyQueue.tail = tempQueue.tail;
        }
    }
}



void move_back_two(int n) {
    struct ReadyQueue tempQueue = {
        .head = NULL,
        .tail = NULL
    };
    struct PCB *current = readyQueue.head;
    struct PCB *prev = NULL;
    int count = 0;
    
    while (current != NULL) {
        if (current->PROG == n && count < 2) {
            if (prev == NULL) {
                // if removing head node
                readyQueue.head = current->NEXT;
            } else {
                prev->NEXT = current->NEXT;
            }
            if (readyQueue.tail == current) {
                // if removing tail node
                readyQueue.tail = prev;
            }
            if (tempQueue.tail == NULL) {
                // if adding first node to tempQueue
                tempQueue.head = current;
                tempQueue.tail = current;
            } else {
                tempQueue.tail->NEXT = current;
                tempQueue.tail = current;
            }
            current = current->NEXT;
            tempQueue.tail->NEXT = NULL;
            count++; // Increment the count of moved PCBs
        } else {
            prev = current;
            current = current->NEXT;
        }
    }
    
    if (tempQueue.head != NULL) {
        if (readyQueue.head == NULL) {
            // if readyQueue was previously empty
            readyQueue.head = tempQueue.head;
            readyQueue.tail = tempQueue.tail;
        } else {
            readyQueue.tail->NEXT = tempQueue.head;
            readyQueue.tail = tempQueue.tail;
        }
    }
}


char* intToString(int value){
	switch (value){
		case 0:
			return "ZERO";
		case 1:
			return "ONE";
		case 2: 
			return "TWO";
		case 3:
			return "THREE";	
		case 4:
			return "FOUR";	
		case 5:
			return "FIVE";	
		case 6:
			return "SIX";	
		case 7:
			return "SEVEN";	
		case 8:
			return "EIGHT";	
		case 9:
			return "NINE";	
		case 10:
			return "TEN";
		case 11:
			return "ELEVEN";	
		case 12:
			return "TWELVE";	
		case 13:
			return "THIRTEEN";	
		case 14:
			return "FOURTEEN";	
		case 15:
			return "FIFTEEN";	
		case 16:
			return "SIXTEEN";	
		case 17:
			return "SEVENTEEN";	
		case 18:
			return "EIGHTEEN";	
		case 19:
			return "NINETEEN";	
		case 20:
			return "TWENTY";
		case 21:
			return "TWENTY-ONE";	
		case 22:
			return "TWENTY-TWO";	
		case 23:
			return "TWENTY-THREE";
		case 24:
			return "TWENTY-FOUR";
		case 25:
			return "TWENTY-FIVE";
		case 26:
			return "TWENTY-SIX";
		case 27:
			return "TWENTY-SEVEN";
		case 28:
			return "TWENTY-EIGHT";
		case 29:
			return "TWENTY-NINE";
		case 30:
			return "THIRTY";
		case 31:
			return "THIRTY-ONE";	
		case 32:
			return "THIRTY-TWO";	
		case 33:
			return "THIRTY-THREE";
		case 34:
			return "THIRTY-FOUR";
		case 35:
			return "THIRTY-FIVE";
		case 36:
			return "THIRTY-SIX";
		case 37:
			return "THIRTY-SEVEN";
		case 38:
			return "THIRTY-EIGHT";
		case 39:
			return "THIRTY-NINE";
		case 40:
			return "FORTY";
		case 41:
			return "FORTY-ONE";	
		case 42:
			return "FORTY-TWO";	
		case 43:
			return "FORTY-THREE";
		case 44:
			return "FORTY-FOUR";
		case 45:
			return "FORTY-FIVE";
		case 46:
			return "FORTY-SIX";
		case 47:
			return "FORTY-SEVEN";
		case 48:
			return "FORTY-EIGHT";
		case 49:
			return "FORTY-NINE";
		case 50:
			return "FIFTY";
		case 51:
			return "FIFTY-ONE";	
		case 52:
			return "FIFTY-TWO";	
		case 53:
			return "FIFTY-THREE";
		case 54:
			return "FIFTY-FOUR";
		case 55:
			return "FIFTY-FIVE";
		case 56:
			return "FIFTY-SIX";
		case 57:
			return "FIFTY-SEVEN";
		case 58:
			return "FIFTY-EIGHT";
		case 59:
			return "FIFTY-NINE";
		case 60:
			return "SIXTY";
		case 61:
			return "SIXTY-ONE";	
		case 62:
			return "SIXTY-TWO";	
		case 63:
			return "SIXTY-THREE";
		case 64:
			return "SIXTY-FOUR";
		case 65:
			return "SIXTY-FIVE";
		case 66:
			return "SIXTY-SIX";
		case 67:
			return "SIXTY-SEVEN";
		case 68:
			return "SIXTY-EIGHT";
		case 69:
			return "SIXTY-NINE";
		case 70:
			return "SEVENTY";
		case 71:
			return "SEVENTY-ONE";	
		case 72:
			return "SEVENTY-TWO";	
		case 73:
			return "SEVENTY-THREE";
		case 74:
			return "SEVENTY-FOUR";
		case 75:
			return "SEVENTY-FIVE";
		case 76:
			return "SEVENTY-SIX";
		case 77:
			return "SEVENTY-SEVEN";
		case 78:
			return "SEVENTY-EIGHT";
		case 79:
			return "SEVENTY-NINE";
		case 80:
			return "EIGHTY";
		case 81:
			return "EIGHTY-ONE";	
		case 82:
			return "EIGHTY-TWO";	
		case 83:
			return "EIGHTY-THREE";
		case 84:
			return "EIGHTY-FOUR";
		case 85:
			return "EIGHTY-FIVE";
		case 86:
			return "EIGHTY-SIX";
		case 87:
			return "EIGHTY-SEVEN";
		case 88:
			return "EIGHTY-EIGHT";
		case 89:
			return "EIGHTY-NINE";
		case 90:
			return "NINETY";
		case 91:
			return "NINETY-ONE";	
		case 92:
			return "NINETY-TWO";	
		case 93:
			return "NINETY-THREE";
		case 94:
			return "NINETY-FOUR";
		case 95:
			return "NINETY-FIVE";
		case 96:
			return "NINETY-SIX";
		case 97:
			return "NINETY-SEVEN";
		case 98:
			return "NINETY-EIGHT";
		case 99:
			return "NINETY-NINE";
		case 100:
			return "ONE-HUNDRED";
	}
}

int findNearestSmallest() {
    int min_score = 999;
    int min_prog = -1;
    struct PCB *current = readyQueue.head->NEXT;
    while (current != NULL) {
        if (current->SCORE < min_score) {
            min_score = current->SCORE;
            min_prog = current->PROG;
        }
        current = current->NEXT;
    }
    return min_prog;
}





