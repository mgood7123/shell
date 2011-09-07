#ifndef _COMMON_H
#define _COMMON_H

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
//need? char *command;
    process *first_process;
    pid_t pgid;
    int notified:1;
    ctruct termious tmodes;
    int stdin, stdout, stderr;
} job;

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

/* for setenv() */
#define _POSIX_C_SOURCE 200112L

/*
#define PRINT_PARSED_COMMAND
#define DEBUG_PARSER
#define DEBUG_RUNNER
*/

#define TERM_DESC STDIN_FILENO

#endif
