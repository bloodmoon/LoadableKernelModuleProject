// C Program to design a shell in Linux 
#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h> 
#include"headers.h"

#define MAXCOM 1000 // max number of letters to be supported 
#define MAXLIST 100 // max number of commands to be supported 

// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J") 
//Define bold green and reset fonts for shell (to look more realistic)
#define ANSI_COLOR_GREEN "\x1b[92m \x1b[1m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_WHITE "\x1b[0m \x1b[1m"
//Char Array for displaying current directory in fake terminal
char currentDir[100]="~"; 
shared_struct *ptr;	// base address, from mmap()
int shm_fd;	// file descriptor, from shm_open()
int size;
//Appends Directory string to display to make it look like normal shell (nothing changed)
const char* name = "Shared Mem";
void appendDir(char* newDir){
	strcat(currentDir,"/");
	strcat(currentDir,newDir);
	strcat(currentDir,"$");
}
	 
// Function to take input and send to parse command 
int takeInput(char* str) 
{ 
	//buffer to be execueted
	char* buf;
	//Get user name to be displayed
	char* username = getenv("USER");
	
	//variable thats printed before command inputs
	//(includes newline, usermame, current directory adn @Virtualbox~)
	char testDir[250]; 	
 	
	//Spoof the directory to display 
	strcpy(testDir,"\n");
	strcat(testDir, username);
	strcat(testDir,"@");		
	strcat(testDir,currentDir);
	
	//Read input
	buf = readline("");    
       
	//Set fonts and colors to spoof command line in terminal 
	printf(ANSI_COLOR_GREEN "%s"ANSI_COLOR_WHITE ":" ANSI_COLOR_RESET,testDir );
	//if buffer input is not empty, execuete strcpy
	if (strlen(buf) != 0) { 		
		strcpy(str, buf); 
		return 0; 
	} else { 
		return 1; 
	} 
} 


// Function where the system command is executed 
void execArgs(char** parsed) 
{ 
	// Forking a child 
	pid_t pid = fork(); 

	if (pid == -1) { 
		printf("\nFailed forking child.."); 
		return; 
	//if fork is succesful execuete parsed command 
	} else if (pid == 0) { 
		if (execvp(parsed[0], parsed) < 0) { 
			printf("\nCould not execute command.."); 
		} 
		exit(0); 
	//main program should wait until the child finishes execueting commands 
	} else { 
		// waiting for child to terminate 
		wait(NULL); 
		return; 
	} 
} 

// Function where the piped system commands is executed 
void execArgsPiped(char** parsed, char** parsedpipe) 
{ 
	// 0 is read end, 1 is write end 
	int pipefd[2]; 
	pid_t p1, p2; 

	if (pipe(pipefd) < 0) { 
		printf("\nPipe could not be initialized"); 
		return; 
	} 
	p1 = fork(); 
	if (p1 < 0) { 
		printf("\nCould not fork"); 
		return; 
	} 

	if (p1 == 0) { 
		// Child 1 executing.. 
		// It only needs to write at the write end 
		close(pipefd[0]); 
		dup2(pipefd[1], STDOUT_FILENO); 
		close(pipefd[1]); 

		if (execvp(parsed[0], parsed) < 0) { 
			printf("\nCould not execute command 1.."); 
			exit(0); 
		} 
	} else { 
		// Parent executing 
		p2 = fork(); 

		if (p2 < 0) { 
			printf("\nCould not fork"); 
			return; 
		} 

		// Child 2 executing.. 
		// It only needs to read at the read end 
		if (p2 == 0) { 
			close(pipefd[1]); 
			dup2(pipefd[0], STDIN_FILENO); 
			close(pipefd[0]); 
			if (execvp(parsedpipe[0], parsedpipe) < 0) { 
				printf("\nCould not execute command 2.."); 
				exit(0); 
			} 
		} else { 
			// parent executing, waiting for two children 
			wait(NULL); 
			wait(NULL); 
		} 
	} 
}
// Function to execute builtin commands 
int ownCmdHandler(char** parsed) 
{ 
	int NoOfOwnCmds = 6, i, switchOwnArg = 0; 
	char* ListOfOwnCmds[NoOfOwnCmds]; 
	char* username; 
	FILE *fp;
	char* fileName;
	//custom commands can be added here 
	ListOfOwnCmds[0] = "exit"; 
	ListOfOwnCmds[1] = "cd"; 
	ListOfOwnCmds[2] = "help"; 
	ListOfOwnCmds[3] = "hello"; 
	ListOfOwnCmds[4] = "browns";
        ListOfOwnCmds[5] = "mkdir";
	//if parsed[0] is equal to a custom command, go to switch statement
	for (i = 0; i < NoOfOwnCmds; i++) { 
		if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) { 
			switchOwnArg = i + 1; 
			break; 
		} 
	} 
	//will be execueted if parsed[0] mathces input command
	switch (switchOwnArg) { 
	case 1: 
		printf("\nGoodbye\n"); 
		exit(0); 
	case 2: 
		chdir(parsed[1]); 
		appendDir(parsed[1]);
		return 1; 
	case 3: 
		return 1; 
	case 4: 
		username = getenv("USER"); 
		printf("\nHello %s.\nMind that this is "
			"not a place to play around."
			"\nUse help to know more..\n", 
			username); 
		return 1; 
	//case 5 and 6 are examples of new commands or overriding old commands
	case 5:
		printf("They Stink\n");
		return 1;
	//THis command changes mkdir so that a .txt file is created instead of a new directory
	//(Proof of concept)
        case 6:
		strcpy(fileName,parsed[1]);
		strcat(fileName,".txt");
		fp = fopen(fileName,"w");
	        fprintf(fp,"You thought this would be a directory didn't you\n");
                 fclose(fp);
		return 1;
      
	default: 
		break; 
	} 

	return 0; 
} 

// function for finding pipe 
int parsePipe(char* str, char** strpiped) 
{ 
	int i; 
	for (i = 0; i < 2; i++) { 
		strpiped[i] = strsep(&str, "|"); 
		if (strpiped[i] == NULL) 
			break; 
	} 

	if (strpiped[1] == NULL) 
		return 0; // returns zero if no pipe is found. 
	else { 
		return 1; 
	} 
} 

// function for parsing command words 
void parseSpace(char* str, char** parsed) 
{ 
	int i; 

	for (i = 0; i < MAXLIST; i++) { 
		parsed[i] = strsep(&str, " "); 

		if (parsed[i] == NULL) 
			break; 
		if (strlen(parsed[i]) == 0) 
			i--; 
	} 
} 

int processString(char* str, char** parsed, char** parsedpipe) 
{ 

	char* strpiped[2]; 
	int piped = 0; 

	piped = parsePipe(str, strpiped); 

	if (piped) { 
		parseSpace(strpiped[0], parsed); 
		parseSpace(strpiped[1], parsedpipe); 

	} else { 

		parseSpace(str, parsed); 
	} 

	if (ownCmdHandler(parsed)) 
		return 0; 
	else
		return 1 + piped; 
} 

int main(/*int argc, char **argv*/) 
{ 
	//builds arrays for input to be put in
	char inputString[MAXCOM], *parsedArgs[MAXLIST];
	char remoteCmd[100]; 
	char* parsedArgsPiped[MAXLIST]; 
	int execFlag = 0;	
	pid_t  pid;

	/* fork another process */
	pid = fork();

	if (pid < 0) { /* error occurred */
		printf("Fork Failed\n");
		exit(-1);
	}
	else if (pid ==0) {
        /* child process */
		sleep(1);
        /* execute server */
		execlp("./serverhideMe","child",NULL);
	}
	else {

        //Create Shared Memory		 
  	shm_fd = shm_open(name, O_CREAT|O_RDWR, 0666);
        size = sizeof(shared_struct);
  	if (shm_fd == -1) {
    		printf("Shared memory failed\n");
    		exit(1);
  	}
	ftruncate(shm_fd,size);
  	/* map the shared memory segment to the address space of the process */
  	ptr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
  	if (ptr == MAP_FAILED) {
    		printf("Map failed\n");
   		 exit(1);
  	}
 
	/* initialize in and out */
        ptr->in = 0;
        ptr->out = 0;	

	while (1) { 
		
		// take input (or retreive from shared memory)
		if(ptr->in != ptr->out)
		{
	   	  command cmd;
		  cmd = ptr->buffer[0];		  		  
		  strcpy(inputString, cmd.data);
		  inputString[strcspn(inputString, "\n")] = 0; 
		  ptr->out = 0;
		  
		}else if (takeInput(inputString)) 
			continue; 
		// process 
		execFlag = processString(inputString, 
		parsedArgs, parsedArgsPiped); 
		// execflag returns zero if there is no command 
		// or it is a builtin command, 
		// 1 if it is a simple command 
		// 2 if it is including a pipe. 

		// execute 
		if (execFlag == 1) 
			execArgs(parsedArgs); 

		if (execFlag == 2) 
			execArgsPiped(parsedArgs, parsedArgsPiped); 
	} 
	}
	shm_unlink(name);
	return 0; 
} 
