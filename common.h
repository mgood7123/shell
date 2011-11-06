#ifndef COMMON_H_SENTRY
#define COMMON_H_SENTRY

/* for setenv () */
#define _POSIX_C_SOURCE 200112L

/* for wait4 () */
#define _BSD_SOURCE

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Exit status, if one of next system calls failed:
 * fork (), pipe (), wait4 (), setpgid (), tcsetpgrp ().
 * Used both in child and shell processes. */
#define ES_SYSCALL_FAILED 1

#define TCSETPGRP_ERROR(value) (((value) == -1) ? 1 : 0)

void set_sig_ign (void);
void set_sig_dfl (void);
void print_prompt1 (void);
void print_prompt2 (void);

#endif
