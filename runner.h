#ifndef _RUNNER_H
#define _RUNNER_H

#include "common.h"
#include <sys/types.h>

typedef struct process {
    char **argv;
    pid_t pid;
    unsigned int completed:1;
    unsigned int stopped:1;
    int status;
    int infile;
    int outfile;
    struct process *next;
} process;

typedef struct job {
/*  Need? char *command; */
    process *first_process;
    pid_t pgid;
    unsigned int notified:1;
/*    struct termious tmodes;
 *    Not compiled with it */
    int stdin;
    int stdout;
    /* int stderr; */
    struct job *next;
} job;

#include "parser.h"
void run_cmd_list (shell_info *sinfo, cmd_list *list);

#endif
