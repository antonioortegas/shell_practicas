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
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job *bgtasks;

void handler(){ // when one of its children exits or stops.
	job *task;
	int pid_wait;
	int status;
	int info;
	enum status status_res;

	print_job_list(bgtasks);

	for(int i = 1; i <= list_size(bgtasks); i++){ // walk the array and see if any process has changed
		task = get_item_bypos(bgtasks, i); // get element i from the list
		pid_wait = waitpid(task->pgid, &status, WUNTRACED | WNOHANG); // Bitwise '|' to set both flags
		if(pid_wait > 0){ // if a child has exited or stopped
			status_res = analyze_status(status, &info);
			if(info == 255){ // go back to prompt if info has an error value
				// Error, command not found: <command>
				printf("Error, command not found: %s\n", task->command);
				continue;
			}
			printf("Background pid: %d, command: %s, status: %s, info: %d\n", pid_wait, task->command, status_strings[status_res], info);
			if(status_res == SUSPENDED){
				task->state = STOPPED;
			} else {
				delete_job(bgtasks, task);
			}
		}
		//wait for zombie processes
		
		
	}

	print_job_list(bgtasks);
}

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

	ignore_terminal_signals();

	// signal SIGCHLD default behaviour is changed
	// SIGCHLD is sent to the parent when any of its children exits or stops.
	signal(SIGCHLD, handler);

	bgtasks = new_list("bgstasks");

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

		// Handle internal commands
		if(strcmp(args[0], "cd") == 0){
			chdir(args[1]);
			continue;
		}

		pid_fork = fork();
		if(pid_fork > 0 ){
			// Parent process
			if(background == 0){ // if command has to be executed in foreground
				// wait for child process to exit
				pid_wait = waitpid(pid_fork, &status, WUNTRACED); // return also if child has stopped instead of exited.
				tcsetpgrp(STDIN_FILENO, getpgrp());
				// Print:
				// Foreground pid: <pid>, command: <command>, <Status>, info: <signal>
				status_res = analyze_status(status, &info);
				if(info == 255){ // go back to prompt if info has an error value
					// Error, command not found: <command>
					printf("Error, command not found: %s\n", args[0]);
					continue;
				}
				printf("Foreground pid: %d, command: %s, status: %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
				if(status_res == SUSPENDED){
					add_job(bgtasks, new_job(getpid(), args[0], SUSPENDED));
				}
			} else { // execute in background
				// Print
				// Background job running... pid: <pid>, command: <command>
				printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);
				// Add to bgtasks list
				add_job(bgtasks, new_job(getpid(), args[0], BACKGROUND));
			}
		} else {
			setpgid(getpid(), getpid()); //set group process id to process id
			if(background == 0){ // give terminal to group if foreground process
				tcsetpgrp(STDIN_FILENO, getpgrp());
			}
			restore_terminal_signals();
			// Child process
			// invoke execvp with given command and arguments
			execvp(args[0], args);
			// if execvp does not find the binary, exit with an error
			exit(-1);
		}
	} // end while
}
