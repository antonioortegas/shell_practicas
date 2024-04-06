/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/
		/* print all arguments from the parser for debugging purposes
		for(int i = 0; args[i]; i++){
			printf("Arg %d: %s\n", i, args[i]);
		}
		*/

		pid_fork = fork();
		if(pid_fork > 0 ){
			// Parent process
			if(background == 0){ // if command has to be executed in foreground
				// wait for child process to exit
				pid_wait = waitpid(pid_fork, &status, 0);
				// Print:
				// Foreground pid: <pid>, command: <command>, <Status>, info: <signal>
				status_res = analyze_status(status, &info);
				if(info == 255){ // go back to prompt if info has an error value
					// Error, command not found: <command>
					printf("Error, command not found: %s\n", args[0]);
					continue;
				}
				printf("Foreground pid: %d, command: %s, status: %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
			} else { // execute in background
				// Print
				// Background job running... pid: <pid>, command: <command>
				printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);
			}
		} else {
			// Child process
			// invoke execvp with given command and arguments
			execvp(args[0], args);
			// if execvp does not find the binary, exit with an error
			exit(-1);
		}
	} // end while
}
