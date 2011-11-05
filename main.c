#include "common.h"
#include "parser.h"
#include "runner.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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

/* Shell initialization:
 * save envp;
 * set PWD enviroment variable;
 * change umask;
 * ignore some signals;
 * get shell group id
 * put shell group to foreground */

/* Note: this is not check for
 * interactively/background runned shell. */
void init_shell (shell_info *sinfo, char **envp)
{
    char *cur_dir = getcwd (NULL, 0);
    if (cur_dir != NULL) {
        setenv ("PWD", cur_dir, 1);
        free (cur_dir);
    }
    umask (S_IWGRP | S_IWOTH);

    /* sinfo = (shell_info *) malloc (sizeof (shell_info)); */
    sinfo->envp = envp;
    sinfo->shell_pgid = getpid ();
    sinfo->orig_stdin = STDIN_FILENO;
    sinfo->orig_stdout = STDOUT_FILENO;
    sinfo->first_job = NULL;
    sinfo->last_job = NULL;

    sinfo->shell_interactive = isatty (sinfo->orig_stdin);
    if (sinfo->shell_interactive) {
        set_sig_ign ();
        tcsetpgrp (sinfo->orig_stdin, sinfo->shell_pgid);
    }
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

int main (int argc, char **argv, char **envp)
{
    shell_info sinfo;
    cmd_list *list;
    parser_info pinfo;

    init_shell (&sinfo, envp);
    init_parser (&pinfo);

    do {
        update_jobs_status (&sinfo);
        print_prompt1 ();
        list = parse_cmd_list (&pinfo);

        switch (pinfo.error) {
        case 0:
#if 1
            run_cmd_list (&sinfo, list);
#else
            print_cmd_list (stdout, list, 1);
            destroy_cmd_list (list);
#endif
            list = NULL;
            break;
        case 16:
#ifdef PARSER_DEBUG
            fprintf (stderr, "Parser: empty command;\n");
#endif
            /* Empty command is not error */
            pinfo.error = 0;
            break;
        default:
            /* TODO: flush read buffer,
             * possibly via buffer function. */
            fprintf (stderr, "Parser: bad command;\n");
            break;
        }

        if (pinfo.cur_lex->type == LEX_EOFILE)
            exit (pinfo.error);
    } while (1);

    return 0;
}
