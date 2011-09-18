#ifndef _COMMON_H
#define _COMMON_H

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

typedef struct process {
    struct process *next;
    char **argv;
    pid_t pid;
    int completed:1;
    int stopped:1;
    int status;
} process;

typedef struct job {
    struct job *next;
/*  Need? char *command; */
    process *first_process;
    pid_t pgid;
    int notified:1;
/*    struct termious tmodes;
 *    Not compiled with it */
    int stdin, stdout, stderr;
} job;

/* To remove
typedef struct pipeline {
	char **argv;
	char *input;
	char *output;
	int append:1;
	struct pipeline *next;
} pipeline;

typedef struct commands_list {
	struct pipeline *pl;
	struct command *cmds;
	int relation;
	struct commands_list *next;
} commands_list;

typedef struct command {
	struct commands_list *list;
	int background;
} command;
*/

/*
#define PRINT_PARSED_COMMAND
#define DEBUG_PARSER
#define DEBUG_RUNNER
*/

#define TERM_DESC STDIN_FILENO

#endif
