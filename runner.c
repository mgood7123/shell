#include "runner.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define NOT_RUNNED -1
#define FAILED -2

#define dup2_or_return(fd1, fd2); \
do {\
	if (dup2(fd1, fd2) < 0) {\
		perror("dup2");\
		return NOT_RUNNED;\
	} \
} while (0);

#define close_or_return(fd); \
do {\
	if (close(fd)) {\
		perror("close");\
		return NOT_RUNNED;\
	} \
} while (0);

#define if_true_return(var); \
do {\
	if ((var)) {\
		perror(#var);\
		return NOT_RUNNED;\
	} \
} while (0);

#define if_zero_return(var); \
do {\
	if (!(var)) {\
		perror(#var);\
		return NOT_RUNNED;\
	} \
} while (0);

#define if_non_zero_exit(var); \
do {\
	if ((var)) {\
		perror(#var);\
		exit(1);\
	} \
} while (0);

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
		printf("%s %s %s\n", "Variable", varname, "is unset!");
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

int change_redirection(pipeline *scmd, int *fd_origin,
		int pipefd[], int input)
{
	int flags, fd;
	int fd_origin_num = input ? 0 : 1;
	char *file = input ? scmd->input : scmd->output;

#ifdef DEBUG_RUNNER
	printf("{%d, %d}\n", pipefd[0], pipefd[1]);
#endif

	/* need redirection? */
	if (file == NULL)
		return 0;
	if (input && pipefd[0] >= 0)
		return 0;
	if (!input && pipefd[1] >= 0)
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

	 /* save copy */
	if_true_return((*fd_origin = dup(fd_origin_num)) < 0);
	/* duplicate a file descriptor */
	dup2_or_return(fd, fd_origin_num);
	/* close file descriptor */
	close_or_return(fd);

	return 0;
}

int unchange_redirection(int fd, int input)
{
	int fd_origin_num = input ? 0 : 1;

	/* need unchange? */
	if (fd < 0)
		return 0;

	close_or_return(fd_origin_num);
	dup2_or_return(fd, fd_origin_num);
	close_or_return(fd);

	return 0;
}

int run_simple_command(pipeline *scmd, int background,
		int pgid, int pipefd[])
{
	char *file;
	pid_t pid;
	int status = 0;
	int fd_in = -1;
	int fd_out = -1;

/* Need?
	if (scmd == NULL
	|| scmd->file == NULL
	|| scmd->argv == NULL
	|| *(scmd->argv) == NULL)
		return NOT_RUNNED;
*/

	file = *(scmd->argv);

	if (change_redirection(scmd, &fd_in, pipefd, 1) != 0
		|| change_redirection(scmd, &fd_out, pipefd, 0) != 0)
		return NOT_RUNNED;

	if (!strcmp(file, "cd")) {
		status = run_cd(scmd);
		if (unchange_redirection(fd_in, 1) != 0
			|| unchange_redirection(fd_out, 0) != 0)
			return FAILED;
		return status;
	}

	pid = fork();
	if (pid == -1) {
		perror("fork()");
		return NOT_RUNNED;
	} else if (pid == 0) {
		if_non_zero_exit(setpgid(0, 0/*pgid*/)); /* tmp */
		execvp(file, scmd->argv);
		perror("execvp");
		exit(1);
	}

	if (!background) {
		tcsetpgrp(0, pid/*pgid*/); /* tmp */

		waitpid(pid, &status, WUNTRACED);

		signal(SIGTTOU, SIG_IGN);
		tcsetpgrp(0, getpgrp());
		signal(SIGTTOU, SIG_DFL);
	}

	if (unchange_redirection(fd_in, 1) != 0
		|| unchange_redirection(fd_out, 0) != 0)
		return FAILED;

	return status;
}

int run_pipeline(pipeline *pl, int background)
{
	int status = NOT_RUNNED;
	int pipefd[] = {-1, -1};
	int pgid = getpgrp();
	int in_origin = dup(0);
	int out_origin = dup(1);

	while (pl) {
		int redirect_in = (status != NOT_RUNNED);
		int redirect_out = (pl->next != NULL);

		/* redirect from pipe to input */
		if (redirect_in) {
			dup2_or_return(pipefd[0], 0);
			close_or_return(pipefd[0]);
		} else
			pipefd[0] = -1;

		/* new pipe */
		if (redirect_in || redirect_out)
			if (pipe(pipefd) != 0) {
				perror("pipe()");
				return NOT_RUNNED;
			}

		/* redirect output to pipe */
		if (redirect_out) {
			dup2_or_return(pipefd[1], 1);
			close_or_return(pipefd[1]);
		} else { /* if (pipefd[1] >= 0) {*/
			close_or_return(1);
			dup2_or_return(out_origin, 1);
			pipefd[1] = -1;
		}

		status = run_simple_command(pl, redirect_out || background,
					pgid, pipefd);

		/* close previous read pipe
		 * TODO: make whew cmd dead
		if (pipefd[0] >= 0)
			close_or_return(0);*/

		if (status != 0)
			break;
		pl = pl->next;
#ifdef DEBUG_RUNNER
		fprintf(stderr, "{%d, %d}\n", pipefd[0], pipefd[1]);
#endif
	}

	if (pipefd[0] >= 0) {
		close_or_return(0);
		dup2_or_return(in_origin, 0);
		pipefd[0] = -1;
	}

	return status;
}

int run_command(command *cmd)
{
	if (!cmd->list)
		return NOT_RUNNED;
	return run_pipeline(cmd->list->pl, cmd->background);
}
