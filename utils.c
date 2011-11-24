#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "utils.h"

void set_sig_ign (void)
{
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    /* Do not ignore SIGCHLD! If SIGCHLD set to SIG_IGN,
     * waitpid/wait4 behaviour changed, see `man waitpid`.
     * It noted in POSIX.1-2001. */
}

void set_sig_dfl (void)
{
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
}

void print_prompt1 (void)
{
    char *ps1 = getenv ("PS1");
    char *user = getenv ("USER");
    char *pwd = getenv ("PWD");

    if (ps1) {
        printf ("%s", ps1);
        return;
    }

    if (user)
        printf ("%s ", user);
    if (pwd) {
        char *directory;
        if (strcmp (pwd, "/")) {
            while (*pwd)
                if (*(pwd++) == '/')
                    directory = pwd;
        } else
            directory = pwd;
        printf ("%s ", directory);
    }

    if (user)
        printf ("%s ", strcmp(user, "root") ? "$" : "#");
    else
        printf ("%s ", "$");
}

void print_prompt2 (void)
{
    char *ps2 = getenv ("PS2");

    if (ps2) {
        printf ("%s", ps2);
        return;
    }

    printf ("%s ", ">");
}

/*
void print_set (char **envp)
{
    if (envp == NULL)
        return;

    while (*envp != NULL)
        printf ("%s\n", *(envp++));
}
*/

/* =========== */
/* Job control */

void register_job (shell_info *sinfo, job *j)
{
    if (sinfo->first_job == NULL)
        sinfo->last_job = sinfo->first_job = j;
    else
        sinfo->last_job = sinfo->last_job->next = j;
}

void unregister_job (shell_info *sinfo, job *j)
{
    job *prev_j = NULL;
    job *cur_j = sinfo->first_job;
    job *next_j = NULL;

    while (cur_j != NULL) {
        next_j = cur_j->next;

        if (cur_j == j) {
            if (cur_j == sinfo->first_job)
                sinfo->first_job = next_j;
            else
                prev_j->next = next_j;

            if (sinfo->last_job == cur_j)
            /* if (next_j == NULL) *//* equal conditions */
                sinfo->last_job = prev_j;

            break;
        }

        prev_j = cur_j;
        cur_j = next_j;
    }
}


/* Returns:
 * 1, if all processes in job stopped;
 * 0, otherwise. */
int job_is_stopped (job *j)
{
    process *p = j->first_process;

    while (p != NULL) {
        if (!p->stopped)
            return 0;
        p = p->next;
    }

    return 1;
}

/* Returns:
 * 1, if all processes in job completed;
 * 0, otherwise. */
int job_is_completed (job *j)
{
    process *p = j->first_process;

    while (p != NULL) {
        if (!p->completed)
            return 0;
        p = p->next;
    }

    return 1;
}

void mark_job_as_runned (job *j)
{
    process *p = j->first_process;

    while (p != NULL) {
        p->stopped = 0;
        p = p->next;
    }
}
