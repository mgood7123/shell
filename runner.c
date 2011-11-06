#include "runner.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* TODO stdin for "read" (by permissions) operations and stdout for "write" */

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
#ifdef RUNNER_DEBUG
            fprintf (stderr, "Runner: unregistered\
 job with pgid %d.\n", j->pgid);
#endif

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
 * 0, if success.
 * 1, otherwise. */
int exec_cd (char *str)
{
    int status;

    if (str == NULL) {
        fprintf (stderr, "cd to NULL? No!\n");
        return 1;
    }

    status = chdir (str);

    if (status == 0) {
        /* Change variables */
        char *oldpwd = getenv ("PWD");
        char *pwd = getcwd (NULL, 0);
        setenv ("OLDPWD", oldpwd, 1);
        setenv ("PWD", pwd, 1);
        free (pwd);
    } else {
        perror ("cd");
    }

    return status;
}

#define STR_EQUAL(str1, str2) (strcmp ((str1), (str2)) == 0)

int run_cd (process *p)
{
    char *new_dir;
    char *arg1 = *(p->argv + 1);
    int status;

    if (arg1 == NULL) {
        new_dir = getenv ("HOME");
    } else if (STR_EQUAL (arg1, "-")) {
        new_dir = getenv ("OLDPWD");
    } else {
        new_dir = arg1;
    }

    status = exec_cd (new_dir);
    new_dir = getenv ("PWD"); /* full path */

    if (status == 0) {
        printf ("%s\n", new_dir);
    } else {
        fprintf (stderr, "cd failed.\n");
    }

    return status;
}

#define GET_FD_ERROR(get_fd_value) ((get_fd_value) == -1)

/* Open file, free pipeline->input.
 * Returns:
 * fd if all right
 * -1 if error */
int get_input_fd (cmd_pipeline *pipeline)
{
    int flags, fd;

    if (pipeline->input == NULL)
        return STDIN_FILENO;

    flags = O_RDONLY;
    fd = open (pipeline->input, flags);

    if (fd < 0) {
        perror("open ()");
        return -1; /* Error */
    }

    free (pipeline->input);
    pipeline->input = NULL;

    return fd;
}

/* Open file, free pipeline->output.
 * Returns:
 * fd if all right
 * -1 if error */
int get_output_fd (cmd_pipeline *pipeline)
{
    int flags, fd;

    if (pipeline->output == NULL)
        return STDOUT_FILENO;

    if (pipeline->append)
        flags = O_CREAT | O_WRONLY | O_APPEND;
    else
        flags = O_CREAT | O_WRONLY | O_TRUNC;

    fd = open (pipeline->output, flags,
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH);

    if (fd < 0) {
        perror("open ()");
        return -1; /* Error */
    }

    free (pipeline->output);
    pipeline->output = NULL;

    return fd;
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

/* TODO: maybe save status, not exited and exit_status? */

/* Returns:
 * 1, if updated;
 * 0, otherwise. */
int mark_job_status (job *j, pid_t pid, int status)
{
    process *p;

    if (pid <= 0)
        return 0;

    for (; j != NULL; j = j->next) {
        for (p = j->first_process; p != NULL; p = p->next) {
            if (p->pid == pid) {
                p->exited = WIFEXITED (status) ? 1 : 0;
                if (p->exited)
                    p->exit_status = WEXITSTATUS (status);
                p->stopped = WIFSTOPPED (status) ? 1 : 0;
                p->completed = (WIFEXITED (status)
                    || WIFSIGNALED (status)) ? 1 : 0;
#ifdef RUNNER_DEBUG
                fprintf (stderr,
"Runner: process %d: exited: %d,\
 exit_status: %d, stopped: %d, completed: %d\n",
                        p->pid, p->exited,
                        p->exit_status, p->stopped,
                        p->completed);
#endif
                return 1;
            }
        }
    }

    return 0;
}

/* TODO: tmp? */
/*
void wait_for_one_process (pid_t pid, int foreground)
{
    if (foreground) {
        wait4 (pid, NULL, WUNTRACED, NULL);
    }

    do {
        pid = wait4 (WAIT_ANY, NULL, WNOHANG | WUNTRACED, NULL);
    } while (pid > 0);

}
*/

#define SETPGID_ERROR(value) (((value) == -1) ? 1 : 0)
#define TCSETPGRP_ERROR(value) (((value) == -1) ? 1 : 0)

/* Blocking until all processes in active job stopped or completed */
/* TODO: check if background */
void wait_for_job (shell_info *sinfo, job *active_job, int foreground)
{
    int status;
    pid_t pid;

    /* Not runned or runned only internal commands */
    if (active_job->pgid == 0)
        return;

    /* if background do nothing */
    if (!(sinfo->shell_interactive && !foreground)) {
        /* TODO: setattr */

        do {
            pid = wait4 (WAIT_ANY, &status, WUNTRACED, NULL);
            if (pid == -1 && errno != ECHILD) {
                perror ("wait4 ()");
                exit (1);
            }

            if (!mark_job_status (sinfo->first_job, pid, status)) {
#ifdef RUNNER_DEBUG
                fprintf (stderr,
"Runner: wait_for_job (): job status not updated:\
 pid: %d, status: %d\n", pid, status);
#endif
                break;
            }
        } while (!job_is_stopped (active_job)
            && !job_is_completed (active_job));
    }

    /* come back terminal permission */
    if (sinfo->shell_interactive && foreground
            && TCSETPGRP_ERROR (
            tcsetpgrp (STDIN_FILENO, sinfo->shell_pgid)))
    {
            perror ("(In shell process) tcsetpgrp ()");
            exit (1);
    }
}

/* Update structures without blocking */
void update_jobs_status (shell_info *sinfo)
{
    int status;
    pid_t pid;
    job *j;

    do {
        pid = wait4 (WAIT_ANY, &status, WUNTRACED | WNOHANG, NULL);
        if (pid == -1 && errno != ECHILD) {
            perror ("wait4 ()");
            exit (1);
        }

        if (!mark_job_status (sinfo->first_job, pid, status))
            break;

        for (j = sinfo->first_job; j != NULL; j = j->next) {
            /* TODO: print some information */

            if (job_is_completed (j)) {
                fprintf (stderr, "Job with pgid %d completed.\n", j->pgid);
                unregister_job (sinfo, j);
                continue;
            }

            if (job_is_stopped (j)) {
                fprintf (stderr, "Job with pgid %d stopped.\n", j->pgid);
                continue;
            }
        }
    } while (1);
}

/* Replace standart in/out channels,
 * close fd[0] and fd[1] if it not
 * original in/out channels.
 * Keep sinfo->origin_std* actual,
 * see comment in typedef shell_info. */
void replace_std_channels (shell_info *sinfo, int fd[2])
{
#ifdef RUNNER_DEBUG
    fprintf (stderr, "Runner: replace_std_channels () with {%d, %d}\n",
            fd[0], fd[1]);
#endif

    if (fd[0] == STDIN_FILENO) {
        if (sinfo->orig_stdin != STDIN_FILENO) {
            dup2 (sinfo->orig_stdin, STDIN_FILENO);
            close (sinfo->orig_stdin);

            /* Mark that std channel not redirected */
            sinfo->orig_stdin = STDIN_FILENO;
        }
    } else {
        /* Keep original channel, if necessary */
        if (sinfo->orig_stdin == STDIN_FILENO)
            sinfo->orig_stdin = dup (STDIN_FILENO);

        dup2 (fd[0], STDIN_FILENO);
        close (fd[0]);
    }

    if (fd[1] == STDOUT_FILENO) {
        if (sinfo->orig_stdout != STDOUT_FILENO) {
            dup2 (sinfo->orig_stdout, STDOUT_FILENO);
            close (sinfo->orig_stdout);

            /* Mark that std channel not redirected */
            sinfo->orig_stdout = STDOUT_FILENO;
        }
    } else {
        /* Keep original channel, if necessary */
        if (sinfo->orig_stdout == STDOUT_FILENO)
            sinfo->orig_stdout = dup (STDOUT_FILENO);

        dup2 (fd[1], STDOUT_FILENO);
        close (fd[1]);
    }
}

/* Change p->completed to 1, if cmd runned.
 * Internal cmd can not be stopped or be uncompleted. */
void try_to_run_internal_cmd (process *p)
{
    if (STR_EQUAL (*(p->argv), "cd")) {
        p->exit_status = run_cd (p);
        p->stopped = 0;
        p->completed = 1;
        p->exited = 1;
    }
}

/* set process group ID
 * set controlling terminal options
 * set job control signals to SIG_DFL
 * exec => no return */
void launch_process (shell_info *sinfo, process *p,
        pid_t pgid, int change_foreground_group)
{
    if (sinfo->shell_interactive) {
        /* set process group ID
         * we must make it before tcsetpgrp */
        if (SETPGID_ERROR (setpgid (p->pid, pgid))) {
            perror ("(In child process) setpgid");
            exit (1);
        }

        /* we must make in before execvp */
        if (change_foreground_group
                && TCSETPGRP_ERROR (
                tcsetpgrp (sinfo->orig_stdin, pgid)))
        {
            perror ("(In child process) tcsetpgrp ()");
            exit (1);
        }

        set_sig_dfl ();
    }

    execvp (*(p->argv), p->argv);
    perror ("(In child process) execvp");
    exit (1);
}

#define FORK_ERROR(fork_value) ((fork_value) < 0)
#define FORK_IS_CHILD(fork_value) ((fork_value) == 0)
#define FORK_IS_PARENT(fork_value) ((fork_value) > 0)
#define PIPE_SUCCESS(pipe_value) ((pipe_value) == 0)
#define PIPE_ERROR(pipe_value) ((pipe_value) == -1)

void launch_job (shell_info *sinfo, job *j,
        int foreground)
{
    process *p;
    pid_t fork_value;
    int pipefd[2];
    int cur_fd[2];

    for (p = j->first_process; p != NULL; p = p->next) {
        /* get input from previous process
         * or from file (for first process) */
        cur_fd[0] = (p == j->first_process) ?
            j->infile : pipefd[0];

        if (p->next != NULL && PIPE_ERROR (pipe (pipefd))) {
            perror ("pipe ()");
            exit (1); /* Or continue work? */
        }

#ifdef RUNNER_DEBUG
        if (p->next != NULL)
            fprintf (stderr, "Runner: pipefd: {%d, %d}\n",
                    pipefd[0], pipefd[1]);
#endif

        /* put output to next process (to pipe)
         * or to file (for last process) */
        cur_fd[1] = (p->next == NULL) ?
            j->outfile : pipefd[1];

        replace_std_channels (sinfo, cur_fd);

        try_to_run_internal_cmd (p);
        if (p->completed)
            continue;

        fork_value = fork ();

        if (FORK_ERROR (fork_value)) {
            perror ("fork");
            exit (1); /* Or continue work? */
        }

        /* set p->pid ang pgid for both processes */
        if (FORK_IS_PARENT (fork_value))
            p->pid = fork_value;
        else
            p->pid = getpid ();

        if (sinfo->shell_interactive
            && j->pgid == 0)
        {
            j->pgid = p->pid;
        }

        if (FORK_IS_CHILD (fork_value)) {
            if (p->next != NULL)
                close (pipefd[0]); /* used by next process */
            /* pipefd[1] == cur_fd[1], already closed */
            launch_process (sinfo, p, j->pgid,
                    sinfo->shell_interactive
                    && foreground
                    && p == j->first_process);
            /* No return */
        }

        /* parent process (here and next) */
        if (sinfo->shell_interactive) {
            /* set process group ID
             * we must make it before tcsetpgrp
             * for job */
            if (SETPGID_ERROR (setpgid (p->pid, j->pgid))) {
                perror ("(In child process) setpgid");
                exit (1);
            }
        }
    } /* for */

    cur_fd[0] = STDIN_FILENO;
    cur_fd[1] = STDOUT_FILENO;
    replace_std_channels (sinfo, cur_fd);
}

/* convert pipeline_item to process,
 * free pipeline_item */
process *pipeline_item_to_process (cmd_pipeline_item *simple_cmd)
{
    process *p;
    if (simple_cmd == NULL)
        return NULL;
    p = (process *) malloc (sizeof (process));
    p->argv = simple_cmd->argv;
    free (simple_cmd);
    p->pid = 0; /* Not runned */
    p->completed = 0;
    p->stopped = 0;
    p->exited = 0;
    p->exit_status = 0;
    p->next = NULL;
    return p;
}

job *make_job ()
{
    job *j = (job *) malloc (sizeof (job));
    j->first_process = NULL;
    j->pgid = 0;
    /* j->pgid == 0 if job not runned
     * or runned only internal commands */
    j->notified = 0;
    j->infile = STDIN_FILENO;
    j->outfile = STDOUT_FILENO;
    j->next = NULL;
    return j;
}

void destroy_job (job *j)
{
    process *next;
    process *p = j->first_process;

    while (p != NULL) {
        next = p->next;
        free (p->argv);
        free (p);
        p = next;
    }

    free (j);
}

job *pipeline_to_job (cmd_pipeline *pipeline)
{
    cmd_pipeline_item *scmd = NULL;
    job *j = make_job ();
    process *p = NULL;

    for (scmd = pipeline->first_item; scmd != NULL;
            scmd = scmd->next)
    {
        if (p == NULL) {
            j->first_process = p =
                pipeline_item_to_process (scmd);
        } else {
            p = p->next =
                pipeline_item_to_process (scmd);
        }
    }

    /* TODO: maybe, make redirections in child process?
     * But it will be not works for internal commands */

    j->infile = get_input_fd (pipeline);
    if (GET_FD_ERROR (j->infile)) {
        fprintf (stderr, "Runner: bad input file.");
        /* TODO: goto error, free () */
        return NULL;
    }

    j->outfile = get_output_fd (pipeline);
    if (GET_FD_ERROR (j->outfile)) {
        fprintf (stderr, "Runner: bad output file.");
        /* TODO: goto error, free () */
        return NULL;
    }

    /* Items already freed in loop */
    /* Strings for input/output files already freed in get_*_fd () */
    free (pipeline);

    return j;
}

/* TODO */
void run_cmd_list (shell_info *sinfo, cmd_list *list)
{
    cmd_list_item *cur_item = list->first_item;
    job *j;

    if (list->first_item->rel != REL_NONE) {
        fprintf (stderr, "Runner: currently command lists not implemented.\n");
        return;
    }

    do {
        j = pipeline_to_job (cur_item->pl);
        if (j == NULL)
            return;

        register_job (sinfo, j);
        launch_job (sinfo, j, list->foreground);
        wait_for_job (sinfo, j, list->foreground);
        if (job_is_completed (j)) {
            unregister_job (sinfo, j);
            destroy_job (j);
            j = NULL;
        }

        /*
        if ((cur_item->rel == REL_NONE)
            || (cur_item->rel == REL_OR && j->status != 0)
            || (cur_item->rel == REL_AND && j->status == 0))
            break;
        */

        cur_item = cur_item->next;
    } while (cur_item != NULL); /* to be on the safe side */
}
