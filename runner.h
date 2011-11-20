#ifndef RUNNER_H_SENTRY
#define RUNNER_H_SENTRY

/* Exit status of child process,
 * if command not found.
 * 127 is default value for bash. */
#define ES_EXEC_ERROR 127

/* Exit status, if built-in command found,
 * but arguments incorrect.
 * 2 is default value for bash. */
#define ES_BUILTIN_CMD_UNCORRECT_ARGS 2

/* Exit status, if built-in command found,
 * but have error during to execute */
#define ES_BUILTIN_CMD_ERROR 1

/* Exit status, if one of next system calls failed:
 * fork (), pipe (), wait4 (), setpgid (), tcsetpgrp ().
 * Used both in child and shell processes. */
#define ES_SYSCALL_FAILED 1

#define STR_EQUAL(str1, str2) (strcmp ((str1), (str2)) == 0)
#define CHDIR_ERROR(chdir_value) ((chdir_value) == -1)
#define GET_FD_ERROR(get_fd_value) ((get_fd_value) == -1)
#define SETPGID_ERROR(value) (((value) == -1) ? 1 : 0)
#define TCSETPGRP_ERROR(value) (((value) == -1) ? 1 : 0)
#define FORK_ERROR(fork_value) ((fork_value) < 0)
#define FORK_IS_CHILD(fork_value) ((fork_value) == 0)
#define FORK_IS_PARENT(fork_value) ((fork_value) > 0)
#define PIPE_SUCCESS(pipe_value) ((pipe_value) == 0)
#define PIPE_ERROR(pipe_value) ((pipe_value) == -1)

/* for setenv () and wait4 () */
#define _BSD_SOURCE
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "parser.h"

typedef struct process {
    char **argv;
    pid_t pid;
    unsigned int completed:1;
    unsigned int stopped:1;
    unsigned int exited:1;
    int exit_status;
    /* exit_status correct, if process
     * completed */
    struct process *next;
} process;

typedef struct job {
    char *name; /* for messages */
    process *first_process;
    pid_t pgid;
/* Nessessary?
    unsigned int notified:1;
*/
/* Not compiled with it
    struct termious tmodes;
*/
    int infile;
    int outfile;
    struct job *next;
} job;

typedef struct shell_info {
    char **envp;
    pid_t shell_pgid;
    unsigned int shell_interactive:1;
    int orig_stdin;
    /* == STDIN_FILENO, if standart
     * input channel not redirected,
     * != STDIN_FILENO, otherwise.
     * This values changed by
     * replace_std_channels (). */
    int orig_stdout;
    /* Similary with STDOUT_FILENO */
    job *first_job;
    job *last_job;
    /*job *active_jobs;*/
} shell_info;

#include "utils.h"

void run_cmd_list (shell_info *sinfo,
        cmd_list *list);
void update_jobs_status (shell_info *sinfo);

#endif
