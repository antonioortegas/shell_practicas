/**
UNIX Shell Project

Alumno: Antonio Ortega Santaolalla

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
#include "parse_redir.h"
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job *list;

// status of a process can be { SUSPENDED, SIGNALED, EXITED, CONTINUED};
// job_state can be { FOREGROUND, BACKGROUND, STOPPED };

void handler(int signal){
    //signal is 17 always, since we only handle 1 signal
    //this is executed when any of the child processes exit or are suspended

    //control the access to the list
    block_SIGCHLD();

    job *item = NULL;
    int status;
    int info;
    int index = 1; // LIST STARTS ON 1, I GUESS WE ARE DOING LUA INDEXING NOW
    int pid_wait;
    int status_res;
    int i;
    for(i = 1; i <= list_size(list); i++){
        item = get_item_bypos(list, index);
        //call waitpid with 2 options ( | bitwise operator )
        //WNOHANG so it does not hang waiting for unfinished processes, since some of the jobs are still running
        pid_wait = waitpid(item->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);
        if(pid_wait == item->pgid){
            //if waitpid returns, that job has been updated (suspended or exited)
            status_res = analyze_status(status, &info);
            if(status_res == SUSPENDED){
                printf("El proceso en segundo plano %s con pid %d se ha suspendido\n", item->command, item->pgid);
                //if the process was suspended, update the list with the new information
                item->state = STOPPED;
            } else if(status_res == EXITED){
                printf("El proceso en segundo plano %s con pid %d ha terminado su ejecucion\n", item->command, item->pgid);
                //since the process has finished, we can delete it from our list
                delete_job(list, item);
                i--;
            } else if(status_res == SIGNALED){
                //if i received a signal, they want to end me
                printf("El proceso %s con pid %d ha sido eliminado\n", item->command, item->pgid);
                delete_job(list, item);
                i--;
            } else if(status_res == CONTINUED){
                // continue in background
                printf("El proceso %s con pid %d continua su ejecucion en segundo plano\n", item->command, item->pgid);
                item->state = BACKGROUND;
            } else {
                //should never happen, this is debug just in case
                printf("ESTADO NO CONTROLADO EN HANDLER\n");
            }
        }
    }
    unblock_SIGCHLD();
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

    int fg_command_fork = 0;

    ignore_terminal_signals();

    //install sigchld handler
    signal(SIGCHLD, handler);

    job *item;
    list = new_list("list");

    //redirect
    char *file_in = NULL;
	char *file_out = NULL;
	FILE *f_in = stdin;
	FILE *f_out = stdout;
    
    while (1) /* Program terminates normally inside get_command() after ^D is typed*/
    {
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */
        //function to parse redirections defined in parse_redir.h
        parse_redirections(args, &file_in, &file_out); //parse redir if there are any

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
        if(strcmp(args[0], "jobs") == 0){
            block_SIGCHLD();
            print_job_list(list);
            unblock_SIGCHLD();
            continue;
        }
        if(strcmp(args[0], "fg") == 0){
            block_SIGCHLD();
            int pos = 1;
            if(args[1] != NULL){
                //if user inputs a position
                pos = atoi(args[1]);
            }
            item = get_item_bypos(list, pos);
            // si el item existe
            if(item != NULL){
                fg_command_fork = 1;
                // 1. ceder terminal
                // 2. actualizar lista
                // 3. mandar SIGCONT por si estuviera suspendido
                tcsetpgrp(STDIN_FILENO, item->pgid);
                if(item->state == STOPPED){
                    killpg(item->pgid, SIGCONT);
                }
                pid_fork = item->pgid;
				strcpy(args[0], item->command); // Copiar el nombre del comando original a args[0]
				delete_job(list, item);
            } else {
                perror("Error executing fg, check that arguments are valid");
            }
            unblock_SIGCHLD();
        }
        if(strcmp(args[0], "bg") == 0){
            block_SIGCHLD();
            int pos = 1;
            if(args[1] != NULL){
                //if user inputs a position
                pos = atoi(args[1]);
            }
            item = get_item_bypos(list, pos);
            // si el item existe y esta suspendido, mandar una señal para que continue
            if(item != NULL && item->state == STOPPED){
                // comando valido, actualizar item numero (pos)
                item->state = BACKGROUND;
                killpg(item->pgid, SIGCONT);
                printf("El proceso %s con pid %d continua su ejecucion en segundo plano\n", item->command, item->pgid);
            } else {
                perror("Error executing bg, check that arguments are valid");
            }
            unblock_SIGCHLD();
            continue;
        }

        if(!fg_command_fork){ //fork only if we are not in a fg command
            pid_fork = fork();
        }
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

            //redirect
            if(file_in != NULL){
                f_in = fopen(file_in, "r");
                if(f_in == NULL){
                    perror("Error opening file for input redirection: ");
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(f_in), STDIN_FILENO);
            }
            if(file_out != NULL){
                f_out = fopen(file_out, "w");
                if(f_out == NULL){
                    perror("Error opening file for output redirection: ");
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(f_out), STDOUT_FILENO);
            }
            // invoke execvp()
            if (execvp(args[0], args) == -1) {
                printf("Error, command not found: %s\n", args[0]);
            }
            //if execvp fails, close the file descriptors and exit
            dup2(STDERR_FILENO, STDOUT_FILENO); //restore stdout


            if(file_in != NULL){
                fclose(f_in);
            }
            if(file_out != NULL){
                fclose(f_out);
            }

            exit(EXIT_FAILURE);
        } else {
            // parent
            new_process_group(pid_fork);
            fg_command_fork = 0; //reset fg_command_fork
            if (background) {
                // make a new job and add it to the list
                block_SIGCHLD();
                item = new_job(pid_fork, args[0], BACKGROUND);
                add_job(list, item);
                printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);
                printf("Añadido el proceso %s, con pid %d a la lista de jobs suspendidos\n", args[0], pid_fork);
                unblock_SIGCHLD();
            } else {
                //wait for child to finish
                pid_wait = waitpid(pid_fork, &status, WUNTRACED);  // wait for child process
                //get the terminal back
                tcsetpgrp(STDIN_FILENO, getpid());
                //print info
                status_res = analyze_status(status, &info);
                //if it was suspended, add it to the list
                if(status_res == SUSPENDED){
                    block_SIGCHLD();
                    item = new_job(pid_fork, args[0], STOPPED);
                    add_job(list, item);
                    printf("Añadido el proceso %s, con pid %d a la lista de jobs suspendidos\n", args[0], pid_fork);
                    unblock_SIGCHLD();
                } else {
                    //if it was not suspended, just print the info
                    printf("Foreground pid: %d, command: %s %s info: %d\n", pid_fork, args[0], status_strings[status_res], info);
                }
            }
        }

    }  // end while
}
