#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "interpreter.h"
#include "shellmemory.h"
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"




int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);
void restartShell();
int count_semicolon(char ui[]);
extern int my_mkdir(char* dirname);
FILE *f; // input file

// Start of everything
int main(int argc, char *argv[]) {
	printf("%s\n", "Shell version 1.2 Created January 2023\n");
    printf("Frame Store Size = %d; Variable Store Size = %d\n", FRAME_SIZE, VAR_MEM_SIZE);
    DIR *dir = opendir("backStore");
    if (dir) {
        closedir(dir);
        char cmd[100] = "rm -rf backStore/*";
        system(cmd);
    }
    else my_mkdir("backStore");
    
	char prompt = '$';  				// Shell prompt
	char userInput[MAX_USER_INPUT];		// user's input stored here
	int errorCode = 0;					// zero means no error, default

	//init user input
	for (int i=0; i<MAX_USER_INPUT; i++)
		userInput[i] = '\0';
	
	//init shell memory
	mem_init();
    
    //check if the input is a file redirection
    if (isatty(fileno(stdin))){

        while(1) {		
            printf("%c ",prompt);//print $
            fgets(userInput, MAX_USER_INPUT-1, stdin);
            errorCode = parseInput(userInput);
            if (errorCode == -1) exit(99);	// ignore all other errors
            memset(userInput, 0, sizeof(userInput));
	    }
    }

    else {
        //while the file is not finished reading
        fgets(userInput, MAX_USER_INPUT-1, stdin);
        errorCode = parseInput(userInput);
        memset(userInput, 0, sizeof(userInput));
        
        while(fgets(userInput, MAX_USER_INPUT-1, stdin) != NULL ) {	
            errorCode = parseInput(userInput); // ignore all other errors
            if (errorCode == -1) exit(99);
            memset(userInput, 0, sizeof(userInput));
	    }

        //restart the shell
        restartShell(); 
    }

	
	return 0;

}

int parseInput(char ui[]) {
    //check if there are ; in the command. 
    // extract single command by strtok.
    if(count_semicolon(ui)>0){
        char *p;
        p = strtok(ui,";");
        while(p){
            parseInput(p);
            p=strtok(NULL,";");}
        return 0;
    }

    char tmp[200];
    char *words[100];                            
    int a = 0;
    int b;                            
    int w=0; // wordID    
    int errorCode;

    for(a=0; ui[a]==' ' && a<1000; a++);        // skip white spaces
    while(ui[a] != '\n' && ui[a] != '\0' && a<1000) {
        for(b=0; ui[a]!=';' && ui[a]!='\0' && ui[a]!='\n' && ui[a]!=' ' && a<1000; a++, b++){
            tmp[b] = ui[a];                        
            // extract a word
        }

        tmp[b] = '\0';
        words[w] = strdup(tmp);
        w++;
        if(ui[a] == '\0') break;
        a++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;
}

// count number of ;
int count_semicolon(char ui[]){
    int count=0;
    for(int i=0; ui[i]!='\0'&&i<1000;i++){
        if(ui[i]==';'){
            count++;
        }
    }
    return count;}

// resratring shell helper function
void restartShell() {
    mem_init();
    freopen("/dev/f", "r", stdin); //move pointer to terminal
    int argc = 0;
    char *argv[] = {NULL};
    main(argc, argv); //initialize main function
}


