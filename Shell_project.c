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

#include "job_control.h"  // remember to compile with module job_control.c
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job *list;

void handler(int signal){
    //signal is 17 always, since we only handle 1 signal
    //this is executed when any of the child processes exit or are suspended
    job *item = NULL;
    int status;
    int info;
    int index = 1; // LIST STARTS ON 1, I GUESS WE ARE DOING LUA INDEXING NOW
    int pid_wait;
    int status_res;

    while(index <= list_size(list)){
        item = get_item_bypos(list, index);
        index++;
        //call waitpid with 2 options ( | bitwise operator )
        //WNOHANG so it does not hang waiting for unfinished processes, since some of the jobs are still running
        pid_wait = waitpid(item->pgid, &status, WUNTRACED | WNOHANG);
        if(pid_wait == item->pgid){
            //if waitpid returns, that job has been updated (suspended or exited)
            status_res = analyze_status(status, &info);
            if(status_res == SUSPENDED){
                printf("El proceso en segundo plano %s con pid %d se ha suspendido", item->command, item->pgid);
                fflush;
                //if the process was suspended, update the list with the new information
                item->state = SUSPENDED;
            } else if(status_res == EXITED){
                printf("El proceso en segundo plano %s con pid %d ha terminado su ejecucion", item->command, item->pgid);
                fflush;
                //since the process has finished, we can delete it from our list
                delete_job(list, item);
            }
            break;
        }
    }
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void) {
    new_process_group(getpid());
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2];   /* command line (of 256) has max of 128 arguments */
    // probably useful variables:
    int pid_fork, pid_wait; /* pid for created and waited process */
    int status;             /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info;               /* info processed by analyze_status() */

    ignore_terminal_signals();

    //install sigchld handler
    signal(SIGCHLD, handler);

    job *item;
    list = new_list("list");
    
    while (1) /* Program terminates normally inside get_command() after ^D is typed*/
    {
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */

        if (args[0] == NULL) continue;  // if empty command

        /* the steps are:
                 (1) fork a child process using fork()
                 (2) the child process will invoke execvp()
                 (3) if background == 0, the parent will wait, otherwise continue
                 (4) Shell shows a status message for processed command
                 (5) loop returns to get_commnad() function
        */

        // Handle internal commands
        if (strcmp(args[0], "cd") == 0) {
            if(args[1] == NULL){
                chdir(getenv("HOME"));
            } else if(chdir(args[1]) < 0){
                perror("Error executing cd: ");
            }
            continue;
        }

        pid_fork = fork();
        if (pid_fork < 0) {
            printf("Error when calling fork()");
        } else if (pid_fork == 0) {
            // child
            //put child in new process group
            new_process_group(getpid());
            if(!background){
                //get the terminal
                tcsetpgrp(STDIN_FILENO, getpid());
            }
            restore_terminal_signals();
            // invoke execvp()
            if (execvp(args[0], args) == -1) {
                printf("Error, command not found: %s\n", args[0]);
            }
            exit(EXIT_FAILURE);
        } else {
            // parent
            if (background) {
                // make a new job and add it to the list
                item = new_job(pid_fork, args[0], BACKGROUND);
                add_job(list, item);
                printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);
            } else {
                //wait for child to finish
                pid_wait = waitpid(pid_fork, &status, WUNTRACED);  // wait for child process
                //get the terminal back
                tcsetpgrp(STDIN_FILENO, getpid());
                //print info
                status_res = analyze_status(status, &info);
                if (info != 1) {
                    printf("Foreground pid: %d, command: %s %s info: %d\n", pid_fork, args[0], status_strings[status_res], info);
                }
                //if it was suspended, add it to the list
                if(status_res == SUSPENDED){
                    item = new_job(pid_fork, args[0], BACKGROUND);
                    add_job(list, item);
                    printf("Añadido el proceso %s, con pid %d a la lista de jobs suspendidos\n", args[0], pid_fork);
                }
            }
        }

    }  // end while
}
