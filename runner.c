#include "runner.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* TODO stdin for "read" (by permissions) operations and stdout for "write" */

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

/* Returns:
 * fd if all right
 * 0 if file == NULL
 * -1 if error */
int get_input_fd (cmd_pipeline *pipeline)
{
    int flags, fd;

    if (pipeline->input == NULL)
        return 0; /* file == NULL */

    flags = O_RDONLY;
    fd = open (pipeline->input, flags);

    if (fd < 0) {
        perror("open ()");
        return -1; /* Error */
    }

    free (pipeline->input);
    pipeline->input = NULL;

    return fd; /* All rigtht */
}

/* Returns:
 * fd if all right
 * 0 if file == NULL
 * -1 if error */
int get_output_fd (cmd_pipeline *pipeline)
{
    int flags, fd;

    if (pipeline->output == NULL)
        return 0; /* file == NULL */

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
        return -1;
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

/* Returns:
 * 1, if updated;
 * 0, otherwise. */
int update_status (job *j, pid_t pid, int status)
{
    process *p = j->first_process;

    if (pid <= 0)
        return 0;

    while (p != NULL) {
        if (p->pid == pid) {
            p->status = status;
            p->stopped = WIFSTOPPED (p->status) ? 1 : 0;
            p->completed = !p->stopped;
            return 1;
        }
    }

    return 0;
}

/* tmp? */
void wait_for_one_process (pid_t pid, int foreground)
{
    if (foreground) {
        wait4 (pid, NULL, WUNTRACED, NULL);
    }

    do {
        pid = wait4 (WAIT_ANY, NULL, WNOHANG | WUNTRACED, NULL);
    } while (pid > 0);

}

/* TODO */
void wait_for_job (job *j, int foreground)
{
    int status;
    pid_t pid;

    do {
        pid = wait4 (WAIT_ANY, &status, WUNTRACED, NULL);
    } while (!update_status (j, pid, status)
            && !job_is_stopped (j)
            && !job_is_completed (j));

    /*
    search_by_pid (pid, j, p);
    pid = wait4 (WAIT_ANY, &(p->status), WUNTRACED, NULL);
    update_job (j, p);
    */
}

void change_redirections (shell_info *sinfo, process *p)
{
    if (p->infile != STDIN_FILENO) {
        sinfo->orig_stdin = dup (STDIN_FILENO);
        dup2 (p->infile, STDIN_FILENO);
        close (p->infile);
    }

    if (p->outfile != STDOUT_FILENO) {
        sinfo->orig_stdout = dup (STDOUT_FILENO);
        dup2 (p->outfile, STDOUT_FILENO);
        close (p->outfile);
    }
}

void unchange_redirections (shell_info *sinfo, process *p)
{
    if (p->infile != STDIN_FILENO) {
        dup2 (sinfo->orig_stdin, STDIN_FILENO);
        close (sinfo->orig_stdin);
        sinfo->orig_stdin = STDIN_FILENO;
    }

    if (p->outfile != STDOUT_FILENO) {
        dup2 (sinfo->orig_stdout, STDOUT_FILENO);
        close (sinfo->orig_stdout);
        sinfo->orig_stdout = STDOUT_FILENO;
    }
}

int run_internal_cmd (process *p)
{
    int runned = 0;

    if (STR_EQUAL (*(p->argv), "cd")) {
        p->status = run_cd (p);
        runned = 1;
    }

    return runned;
}

/* set process group ID
 * set controlling terminal options
 * set job control signals to SIG_DFL
 * exec => no return */
void launch_process (shell_info *sinfo, process *p,
        pid_t pgid, int foreground)
{
    if (sinfo->shell_interactive) {
        /* set process group ID
         * we must make it before tcsetpgrp */
        if (setpgid (p->pid, pgid)) {
            perror ("(In child process) setpgid");
            exit (1);
        }

        /* we must make in before execvp */
        if (foreground && tcsetpgrp (sinfo->orig_stdin, pgid)) {
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

void launch_job (shell_info *sinfo, job *j, int foreground)
{
    process *p;
    pid_t fork_value;

    for (p = j->first_process; p != NULL; p = p->next) {
        if (p == j->first_process)
            p->infile = j->stdin;
        if (p->next == NULL)
            p->outfile = j->stdout;

        change_redirections (sinfo, p);

        if (run_internal_cmd (p)) {
            unchange_redirections (sinfo, p);
            continue;
        }

        fork_value = fork ();

        if (FORK_ERROR (fork_value)) {
            perror ("fork");
            exit (1); /* return? */
        }

        /* set p->pid ang pgid for both processes */
        if (sinfo->shell_interactive) {
            if (FORK_IS_PARENT (fork_value)) {
                p->pid = fork_value;
            } else {
                p->pid = getpid ();
            }

            if (p == j->first_process) {
                j->pgid = p->pid;
            }
        }

        if (FORK_IS_CHILD (fork_value)) {
            /* TODO: resolve pipes */
            launch_process (sinfo, p, j->pgid, foreground); /* No return */
        }

        /* parent process (here and next) */
        if (sinfo->shell_interactive) {
            /* set process group ID
             * we must make it before tcsetpgrp
             * for job (after this for) */
            if (setpgid (p->pid, j->pgid))
                perror ("(In child process) setpgid");
        }

        unchange_redirections (sinfo, p);
    } /* for */

    if (sinfo->shell_interactive) {
        if (foreground) {
            /* TODO: setattr */
            /*wait_for_job (j, foreground);*/
            wait_for_one_process (j->pgid, foreground);
        }
        /* if background do nothing */
    } else {
        /* not interactive */
        /*wait_for_job (j, foreground);*/
        wait_for_one_process (j->pgid, foreground);
    }

    /* come back terminal permission */
    /* TODO: if foreground? */
    tcsetpgrp (STDIN_FILENO, sinfo->shell_pgid);
}

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
    p->status = 0;
    p->infile = STDIN_FILENO;
    p->outfile = STDOUT_FILENO;
    p->next = NULL;
    return p;
}

job *pipeline_to_job (cmd_pipeline *pipeline)
{
    job *j = (job *) malloc (sizeof (job));
    process *current_item = NULL;
    cmd_pipeline_item *current_simple_cmd = pipeline->first_item;
    int tmp_fd;

    j->first_process = NULL;
    j->pgid = 0; /* Not runned */
    j->notified = 0;
    j->stdin = STDIN_FILENO;
    j->stdout = STDOUT_FILENO;
    /* j->stderr = STDERR_FILENO; */
    j->next = NULL;

    while (current_simple_cmd != NULL) {
        if (current_item == NULL) {
            j->first_process = current_item =
                pipeline_item_to_process (current_simple_cmd);
        } else {
            fprintf (stderr, "Runner: currently pipelines not implemented.\n");
            return NULL;
            /*
            current_item = current_item->next =
                pipeline_item_to_process (current_simple_cmd);
            */
        }
        current_simple_cmd = current_simple_cmd->next;
    }

    /* TODO: maybe, make redirections in child process? */
    tmp_fd = get_input_fd (pipeline);
    if (tmp_fd < 0) {
        /* TODO: goto error, free () */
        return NULL;
    } else if (tmp_fd > 0) {
        j->stdin = tmp_fd;
    } else {
        j->stdin = STDIN_FILENO;
    }

    tmp_fd = get_output_fd (pipeline);
    if (tmp_fd < 0) {
        /* TODO: goto error, free () */
        return NULL;
    } else if (tmp_fd > 0) {
        j->stdout = tmp_fd;
    } else {
        j->stdout = STDOUT_FILENO;
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

        launch_job (sinfo, j, list->foreground);

        /*
        if ((cur_item->rel == REL_NONE)
            || (cur_item->rel == REL_OR && j->status != 0)
            || (cur_item->rel == REL_AND && j->status == 0))
            break;
        */

        cur_item = cur_item->next;
    } while (cur_item != NULL); /* to be on the safe side */
}
