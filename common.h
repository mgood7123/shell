#ifndef _COMMON_H
#define _COMMON_H

/* for setenv () */
#define _POSIX_C_SOURCE 200112L

/* for wait4 () */
#define _BSD_SOURCE

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct shell_info {
    char **envp;
    pid_t shell_pgid;
    unsigned int shell_interactive:1;
    int orig_stdin;
    int orig_stdout;
} shell_info;

void set_sig_ign (void);
void set_sig_dfl (void);
void print_prompt1 (void);
void print_prompt2 (void);

#endif
