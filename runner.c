#include "runner.h"

/* TODO stdin for "read" (by permissions) operations and stdout for "write" */

void print_job_status(job *j, const char *status)
{
     printf("[id: %d, pgid: %d] (%s ...): %s\n",
             j->id, j->pgid,
             *(j->first_process->argv), status);
}

/* Returns:
 * 0, on success;
 * non-zero value, otherwise */
int run_cd(process *p)
{
    char *new_dir;
    char *arg1 = *(p->argv + 1);
    int print_new_dir = 0;
    char *cur_dir = getcwd(NULL, 0);

    /* Resolve new_dir */
    if (arg1 == NULL) {
        new_dir = getenv("HOME");
    } else if (*(p->argv + 2) != NULL) {
        fprintf(stderr, "cd: too many argumens!\n");
        free(cur_dir);
        return ES_BUILTIN_CMD_UNCORRECT_ARGS;
    } else if (STR_EQUAL(arg1, "-")) {
        new_dir = getenv("OLDPWD");
        print_new_dir = 1;
    } else {
        new_dir = arg1;
    }

    if (new_dir == NULL) {
        fprintf(stderr, "cd: necessary variable is unset!\n");
        free(cur_dir);
        return ES_BUILTIN_CMD_ERROR;
    }

    if (CHDIR_ERROR(chdir(new_dir))) {
        perror("chdir()");
        free(cur_dir);
        return ES_BUILTIN_CMD_ERROR;
    }

    /* Change variables */
    new_dir = getcwd(NULL, 0); /* get full path */
    setenv("OLDPWD", cur_dir, 1);
    free(cur_dir);
    setenv("PWD", new_dir, 1);

    if (print_new_dir)
        printf("%s\n", new_dir);
    free(new_dir);

    return 0;
}

/* Returns:
 * 0, on success;
 * non-zero value, otherwise */
int run_jobs(shell_info *sinfo, process *p)
{
    job *j;

    if (*(p->argv + 1) != NULL) {
        fprintf(stderr, "jobs: too many argumens!\n");
        return ES_BUILTIN_CMD_UNCORRECT_ARGS;
    }

    update_jobs_status(sinfo);
    printf("Jobs list:\n");

    for (j = sinfo->first_job; j != NULL; j = j->next) {

        if (j->id == sinfo->cur_job_id) {
            if (job_is_stopped(j)) {
                print_job_status(j, "stopped; current job");
            } else {
                print_job_status(j, "runned; current job");
            }
        } else {
            if (job_is_stopped(j)) {
                print_job_status(j, "stopped");
            } else {
                print_job_status(j, "runned");
            }
        }
    }

    return 0;
}

void wait_for_job(shell_info *sinfo, job *active_job, int foreground);

/* Returns:
 * 0, if job continued and (if nessassary) waited
 * 1, on any errors. */
int continue_job(shell_info *sinfo, job *j, int foreground)
{
    if (sinfo->shell_interactive && foreground
        && TCSETPGRP_ERROR(
        tcsetpgrp(sinfo->orig_stdin, j->pgid)))
    {
        perror("tcsetpgrp()");
        fprintf(stderr, "Possibly, typed job already");
        fprintf(stderr, " completed.\n");
        return 1;
    }

    if (KILL_ERROR(kill(- j->pgid, SIGCONT))) {
        perror("kill(SIGCONT)");
        return 1;
    }

    mark_job_as_runned(j);

    if (!sinfo->shell_interactive || foreground)
        sinfo->cur_job_id = j->id;

    if (sinfo->shell_interactive && !foreground) {
        print_job_status(j, "continued in background");
    } else {
        print_job_status(j, "continued in foreground");
    }
    wait_for_job(sinfo, j, foreground);

    return 0;
}

/* Returns:
 * 0, on success;
 * non-zero value, otherwise */
int run_bg_fg(shell_info *sinfo, process *p, int foreground)
{
    job *j;
    int typed_id;

    if (*(p->argv + 2) != NULL) {
        fprintf(stderr, "bg/fg: too many arguments!\n");
        return ES_BUILTIN_CMD_UNCORRECT_ARGS;
    }

    /* Get number of job */
    if (*(p->argv + 1) == NULL) {
        if (sinfo->cur_job_id == 0) {
            fprintf(stderr, "bg/fg: shell have not");
            fprintf(stderr, " current job.\n");
            return ES_BUILTIN_CMD_UNCORRECT_ARGS;
        } else {
            typed_id = sinfo->cur_job_id;
        }
    } else {
        typed_id = atoi(*(p->argv + 1));
        /* On all errors atoi returns 0 */
        /* On "0" also return 0, but it is not a problem:
         * 0 is not correct job ID */
    }

    if (typed_id == 0) {
        fprintf(stderr, "bg/fg: uncorrect argument!\n");
        return ES_BUILTIN_CMD_UNCORRECT_ARGS;
    }

    update_jobs_status(sinfo);

    for (j = sinfo->first_job; j != NULL; j = j->next) {
        if (j->id == typed_id) {
            if (continue_job(sinfo, j, foreground) != 0)
                return ES_BUILTIN_CMD_ERROR;
            return 0;
        }
    }

    fprintf(stderr, "bg/fg: job %d not found!\n", typed_id);
    return ES_BUILTIN_CMD_ERROR;
}

/* Open file, free pipeline->input.
 * Returns:
 * fd if all right
 * -1 if error */
int get_input_fd(cmd_pipeline *pipeline)
{
    int flags, fd;

    if (pipeline->input == NULL)
        return STDIN_FILENO;

    flags = O_RDONLY;
    fd = open(pipeline->input, flags);

    if (fd < 0) {
        perror("open()");
        return -1; /* Error */
    }

    free(pipeline->input);
    pipeline->input = NULL;

    return fd;
}

/* Open file, free pipeline->output.
 * Returns:
 * fd if all right
 * -1 if error */
int get_output_fd(cmd_pipeline *pipeline)
{
    int flags, fd;

    if (pipeline->output == NULL)
        return STDOUT_FILENO;

    if (pipeline->append)
        flags = O_CREAT | O_WRONLY | O_APPEND;
    else
        flags = O_CREAT | O_WRONLY | O_TRUNC;

    fd = open(pipeline->output, flags,
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH);

    if (fd < 0) {
        perror("open()");
        return -1; /* Error */
    }

    free(pipeline->output);
    pipeline->output = NULL;

    return fd;
}

/* TODO: maybe save status, not exited and exit_status? */

/* Returns:
 * marked job
 * or NULL, if no job was marked; */
job *mark_job_status(job *j, pid_t pid, int status)
{
    process *p;

    if (pid <= 0)
        return NULL;

    for (; j != NULL; j = j->next) {
        for (p = j->first_process; p != NULL; p = p->next) {
            if (p->pid == pid) {
                p->exited = WIFEXITED(status) ? 1 : 0;
                if (p->exited)
                    p->exit_status = WEXITSTATUS(status);
                p->stopped = WIFSTOPPED(status) ? 1 : 0;
                p->completed = (WIFEXITED(status)
                    || WIFSIGNALED(status)) ? 1 : 0;
                return j;
            }
        }
    }

    return NULL;
}

/* Blocking until all processes in active job stopped or completed.
 * If active job completed, remove it from list */
void wait_for_job(shell_info *sinfo, job *active_job, int foreground)
{
    int status;
    pid_t pid;

    /* Not runned or runned only built-in commands */
    if (active_job->pgid == 0)
        return;

    /* Do wait() only if
     * (shell not interactive || foreground job) */
    if (sinfo->shell_interactive && !foreground)
        return;

    /* TODO: setattr */

    /* wait all processes in job */
    do {
        pid = wait4(WAIT_ANY, &status, WUNTRACED, NULL);
        if (pid == -1 && errno != ECHILD) {
            perror("wait4()");
            exit(ES_SYSCALL_FAILED);
        }

        if (mark_job_status(sinfo->first_job, pid, status) == NULL) {
            break;
        }
        if (job_is_stopped(active_job)) {
            print_job_status(active_job, "stopped");
            break;
        }
        if (job_is_completed(active_job)) {
            unregister_job(sinfo, active_job);
            destroy_job(active_job);
            break;
        }
    } while (1);

    /* Do tcsetpgrp() only if
     * (shell interactive && foreground job) */
    if (!sinfo->shell_interactive || !foreground)
        return;

    /* come back terminal permission */
    if (TCSETPGRP_ERROR(
            tcsetpgrp(STDIN_FILENO, sinfo->shell_pgid)))
    {
            perror("(In shell process) tcsetpgrp()");
            exit(ES_SYSCALL_FAILED);
    }
}

/* Update structures without blocking */
void update_jobs_status(shell_info *sinfo)
{
    int status;
    pid_t pid;
    job *j;

    do {
        pid = wait4(WAIT_ANY, &status, WUNTRACED | WNOHANG, NULL);
        if (pid == -1 && errno != ECHILD) {
            perror("wait4()");
            exit(ES_SYSCALL_FAILED);
        }

        j = mark_job_status(sinfo->first_job, pid, status);
        if (j == NULL)
            break;

        if (job_is_completed(j)) {
            print_job_status(j, "completed");
            unregister_job(sinfo, j);
            continue;
        }
        if (job_is_stopped(j)) {
            print_job_status(j, "stopped");
            continue;
        }
    } while (1);
}

/* Replace standart in/out channels,
 * close fd[0] and fd[1] if it not
 * original in/out channels.
 * Keep sinfo->origin_std* actual,
 * see comment in typedef shell_info. */
void replace_std_channels(shell_info *sinfo, int fd[2])
{
    if (fd[0] == STDIN_FILENO) {
        if (sinfo->orig_stdin != STDIN_FILENO) {
            dup2(sinfo->orig_stdin, STDIN_FILENO);
            close(sinfo->orig_stdin);

            /* Mark that std channel not redirected */
            sinfo->orig_stdin = STDIN_FILENO;
        }
    } else {
        /* Keep original channel, if necessary */
        if (sinfo->orig_stdin == STDIN_FILENO)
            sinfo->orig_stdin = dup(STDIN_FILENO);

        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
    }

    if (fd[1] == STDOUT_FILENO) {
        if (sinfo->orig_stdout != STDOUT_FILENO) {
            dup2(sinfo->orig_stdout, STDOUT_FILENO);
            close(sinfo->orig_stdout);

            /* Mark that std channel not redirected */
            sinfo->orig_stdout = STDOUT_FILENO;
        }
    } else {
        /* Keep original channel, if necessary */
        if (sinfo->orig_stdout == STDOUT_FILENO)
            sinfo->orig_stdout = dup(STDOUT_FILENO);

        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
    }
}

/* Change p->completed to 1, if cmd runned.
 * Internal cmd can not be stopped or be uncompleted. */
void try_to_run_builtin_cmd(shell_info *sinfo, process *p)
{
    int runned = 1;

    if (STR_EQUAL(*(p->argv), "cd"))
        p->exit_status = run_cd(p);
    else
        runned = 0;

    if (runned) {
        p->stopped = 0;
        p->completed = 1;
        p->exited = 1;
    }
}

/* Change j->first_process->completed to 1, if cmd runned or
 * if pipeline contain more then one cmd. In last case set
 * j->first_process->exit_status to ES_BUILTIN_CMD_ERROR.
 * Job control cmd can not be stopped or be uncompleted. */
void try_to_run_job_control_cmd(shell_info *sinfo, job *j)
{
    int runned;
    process *p = j->first_process;

    if (!STR_EQUAL(*(p->argv), "jobs")
        && !STR_EQUAL(*(p->argv), "bg")
        && !STR_EQUAL(*(p->argv), "fg"))
    {
        /* This is not job control command */
        return;
    }

    if (p->next != NULL) {
        fprintf(stderr, "Job control command");
        fprintf(stderr, " can not be runned as");
        fprintf(stderr, " element of pipeline!\n");
        p->stopped = 0;
        p->completed = 1;
        p->exit_status = ES_BUILTIN_CMD_ERROR;
        p->exited = 1;
        return;
    }

    runned = 1;

    if (STR_EQUAL(*(p->argv), "jobs"))
        p->exit_status = run_jobs(sinfo, p);
    else if (STR_EQUAL(*(p->argv), "bg"))
        p->exit_status = run_bg_fg(sinfo, p, 0);
    else if (STR_EQUAL(*(p->argv), "fg"))
        p->exit_status = run_bg_fg(sinfo, p, 1);
    else
        runned = 0;

    if (runned) {
        p->stopped = 0;
        p->completed = 1;
        p->exited = 1;
    }
}

/* set process group ID
 * set controlling terminal options
 * set job control signals to SIG_DFL
 * exec => no return */
void launch_process(shell_info *sinfo, process *p,
        pid_t pgid, int change_foreground_group)
{
    if (sinfo->shell_interactive) {
        /* set process group ID
         * we must make it before tcsetpgrp */
        if (SETPGID_ERROR(setpgid(p->pid, pgid))) {
            perror("(In child process) setpgid");
            exit(ES_SYSCALL_FAILED);
        }

        /* we must make in before execvp */
        if (change_foreground_group
                && TCSETPGRP_ERROR(
                tcsetpgrp(sinfo->orig_stdin, pgid)))
        {
            perror("(In child process) tcsetpgrp()");
            exit(ES_SYSCALL_FAILED);
        }

        set_sig_dfl();
    }

    execvp(*(p->argv), p->argv);
    perror("(In child process) execvp");
    exit(ES_EXEC_ERROR);
}

void launch_job(shell_info *sinfo, job *j,
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

        if (p->next != NULL && PIPE_ERROR(pipe(pipefd))) {
            perror("pipe()");
            exit(ES_SYSCALL_FAILED);
        }

        /* put output to next process (to pipe)
         * or to file (for last process) */
        cur_fd[1] = (p->next == NULL) ?
            j->outfile : pipefd[1];

        replace_std_channels(sinfo, cur_fd);

        try_to_run_builtin_cmd(sinfo, p);
        if (p->completed)
            continue;

        fork_value = fork();

        if (FORK_ERROR(fork_value)) {
            perror("fork");
            exit(ES_SYSCALL_FAILED);
        }

        /* set p->pid ang pgid for both processes */
        if (FORK_IS_PARENT(fork_value))
            p->pid = fork_value;
        else
            p->pid = getpid();

        if (sinfo->shell_interactive
            && j->pgid == 0)
        {
            j->pgid = p->pid;
        }

        if (FORK_IS_CHILD(fork_value)) {
            if (p->next != NULL)
                close(pipefd[0]); /* used by next process */
            /* pipefd[1] == cur_fd[1], already closed */
            launch_process(sinfo, p, j->pgid,
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
            if (SETPGID_ERROR(setpgid(p->pid, j->pgid))) {
                perror("(In child process) setpgid");
                exit(ES_SYSCALL_FAILED);
            }
        }
    } /* for */

    cur_fd[0] = STDIN_FILENO;
    cur_fd[1] = STDOUT_FILENO;
    replace_std_channels(sinfo, cur_fd);
}

job *pipeline_to_job(cmd_pipeline *pipeline)
{
    cmd_pipeline_item *scmd = NULL;
    job *j = make_job();
    process *p = NULL;

    for (scmd = pipeline->first_item; scmd != NULL;
            scmd = scmd->next)
    {
        if (p == NULL) {
            j->first_process = p =
                pipeline_item_to_process(scmd);
        } else {
            p = p->next =
                pipeline_item_to_process(scmd);
        }
    }

    /* TODO: maybe, make redirections in child process?
     * But it will be not works for built-in commands */

    j->infile = get_input_fd(pipeline);
    if (GET_FD_ERROR(j->infile)) {
        fprintf(stderr, "Runner: pipeline_to_job(): bad input file.\n");
        /* TODO: goto error, free() */
        return NULL;
    }

    j->outfile = get_output_fd(pipeline);
    if (GET_FD_ERROR(j->outfile)) {
        fprintf(stderr, "Runner: pipeline_to_job(): bad output file.\n");
        /* TODO: goto error, free() */
        return NULL;
    }

    /* Items already freed in loop */
    /* Strings for input/output files already freed in get_*_fd() */
    free(pipeline);

    return j;
}

/* Choose id, first that not used by other jobs.
 * Id starts from 1. */
void choose_job_id(shell_info *sinfo, job *new_job, int foreground)
{
    job *j;
    int id_in_use;

    new_job->id = 0;

    do {
        ++(new_job->id);
        id_in_use = 0;
        for (j = sinfo->first_job;
                !id_in_use && j != NULL;
                j = j->next)
        {
            id_in_use = (j->id == new_job->id);
        }
    } while (id_in_use);

    if (!sinfo->shell_interactive || foreground)
        sinfo->cur_job_id = new_job->id;
}

/* TODO: lists */
void run_cmd_list(shell_info *sinfo, cmd_list *list)
{
    cmd_list_item *cur_item;
    job *j;

    if (list->first_item->rel != REL_NONE) {
        fprintf(stderr, "Runner: run_cmd_list():\
currently command lists not implemented.\n");
        return;
    }

    for (cur_item = list->first_item;
        cur_item != NULL;
        cur_item = cur_item->next)
    {
        j = pipeline_to_job(cur_item->pl);
        if (j == NULL)
            return;

        /* We not redirect input/output for
         * job control commands */
        try_to_run_job_control_cmd(sinfo, j);
        if (job_is_completed(j) ||
            j->first_process->exit_status != 0)
        {
            destroy_job(j);
            j = NULL;
            continue;
        }

        launch_job(sinfo, j, list->foreground);
        choose_job_id(sinfo, j, list->foreground);
        register_job(sinfo, j);
        if (sinfo->shell_interactive && !list->foreground)
            print_job_status(j, "launched in background");
        wait_for_job(sinfo, j, list->foreground);

        /*
        if ((cur_item->rel == REL_NONE)
            || (cur_item->rel == REL_OR && j->status != 0)
            || (cur_item->rel == REL_AND && j->status == 0))
            break;
        */
    } while (cur_item != NULL); /* to be on the safe side */
}
