#include "runner.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int cd_to_path(char *str)
{
	int status = chdir(str);

	if (status) {
		perror((char *) NULL);
	} else {
		char *oldpwd = getenv("PWD");
		char *pwd = getcwd(NULL, 0);
		setenv("OLDPWD", oldpwd, 1);
		setenv("PWD", pwd, 1);
		free(pwd);
	}

	return status;
}

char *cd_to_var(char *varname)
{
	char *var = getenv(varname);

	if (!var) {
		printf("Variable %s is unset!\n", varname);
		return NULL;
	}

	if (cd_to_path(var))
		return NULL;
	return var;
}

int run_cd(pipeline *scmd)
{
	char *arg2 = *(scmd->argv+1);
	if (arg2) {
		if (strcmp(arg2, "-"))
			return cd_to_path(arg2) == 0;
		else
			return cd_to_var("OLDPWD") != NULL;
	} else {
		char *pwd = cd_to_var("HOME");
		if (pwd != NULL)
			printf("%s\n", pwd);
		return pwd != NULL;
	}
}

int change_redirection(pipeline *scmd, int input)
{
	int flags, fd;
	int fd_origin_num = input ? STDIN_FILENO : STDOUT_FILENO;
	char *file = input ? scmd->input : scmd->output;

	/* need redirection? */
	if (file == NULL)
		return 0;

	/* change flags */
	if (input)
		flags = O_RDONLY;
	else if (scmd->append)
		flags = O_CREAT | O_WRONLY | O_APPEND;
	else
		flags = O_CREAT | O_WRONLY | O_TRUNC;

	/* open file */
	if (input)
		fd = open(file, flags);
	else
		fd = open(file, flags,
			S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP |
			S_IROTH | S_IWOTH);
	if (fd < 0) {
		perror("open()");
		return NOT_RUNNED;
	}

	 /* save copy */ /* not need if in clild process
	if_true_return((*fd_origin = dup(fd_origin_num)) < 0);
	*/

	/* duplicate a file descriptor */
	dup2_or_return(fd, fd_origin_num);
	/* close file descriptor */
	close_or_return(fd);

	return 0;
}

int unchange_redirection(int fd, int input)
{
	int fd_origin_num = input ? STDIN_FILENO : STDOUT_FILENO;

	/* need unchange? */
	if (fd < 0)
		return 0;

	close_or_return(fd_origin_num);
	dup2_or_return(fd, fd_origin_num);
	close_or_return(fd);

	return 0;
}

/* set process group ID
 * set controlling terminal options
 * set job control signals to SIG_DFL
 * exec */
void launch_process(process *p, pid_t pgid, int foreground)
{
    if (shell_interactive) {
        /* set process group ID
         * (0 mean calling process ID) */
        /* we must make it before tcsetpgrp */
        if (setpgid(0, pgid)) {
            perror("(In child process) setpgid");
        }

        /* we must make in before execvp */
        if (foreground && tcsetpgrp(TERM_DESC, pgid)) {
            perror("(In child process) tcsetpgrp");
        }

        set_sig_dfl();
    }
    /* TODO: dup2 && close for redirections */
    execvp(*(p->argv), p->argv);
    perror("(In child process) execvp");
    exit(1);
}

void launch_job(job *j, int foreground)
{
    process *p;
    pid_t fork_value;

    for (p = j->first_process; p; p = p->next) {
        /* TODO: resolve pipes and redirections */
        fork_value = fork();

        if (fork_value < 0) {
            perror("fork");
            exit(1); // return?
        }

        /* set p->pid ang pgid for both processes */
        if (shell_interactive) {
            if (fork_value) {
                /* shell process */
                p->pid = fork_value;
            } else {
                /* child process */
                p->pid = getpid();
            }

            if (p == j->first_process) {
                j->pgid = p->pid;
            }
        }

        if (fork_value == 0) {
            /* child process */
            launch_process(p, j->pgid, foreground);
        }

        /* parent process */

        if (shell_interactive) {
            /* set process group ID
             * we must make it before tcsetpgrp
             * for job (after this for) */
            if (setpgid(p->pid, j->pgid)) {
                perror("(In child process) setpgid");
            }
        }

        /* TODO: unset redirections */
    } /* for */

    if (shell_interactive) {
        if (foreground) {
            tcsetpgrp(p->pid, j->pgid);
            wait_for_job(j);
            /* come back terminal permission
            (0 mean calling process group ID) */
            tcsetpgrp(TERM_DESC, 0);
            /* TODO: setattr */
        }
        /* if background do nothing */
    } else {
        /* not interactive */
        wait_for_job(j);
    }
}

int run_command(command *cmd)
{
	if (!cmd->list)
		return NOT_RUNNED;
	return run_pipeline(cmd->list->pl, cmd->background);
}
