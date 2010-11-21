#ifndef _COMMON_H
#define _COMMON_H

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

#endif
