#ifndef _TYPES_H
#define _TYPES_H

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

#endif
