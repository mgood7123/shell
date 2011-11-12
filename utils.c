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
