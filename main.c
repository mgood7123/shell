#include "common.h"
#include "lexer.h"
/* #include "runner.h" */

/* for setenv () */
#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

/*
char **global_envp;
pid_t shell_pgid;

void set_sig_ign(void)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
}

void set_sig_dfl(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
}
*/

/* Shell initialization:
 * save envp;
 * set PWD enviroment variable;
 * change umask;
 * ignore some signals;
 * get shell group id
 * put shell group to foreground */

/* Note: this is not check for
 * interactively/background runned shell. */
/*
void init_shell(char **envp)
{
	char *pwd;

	global_envp = envp;

	pwd = getcwd(NULL, 0);
	setenv("PWD", pwd, 1);
	free(pwd);

	umask(S_IWGRP | S_IWOTH);

	set_sig_ign();

	shell_pgid = getpid();
	tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void print_prompt1(void)
{
	char *ps1 = getenv("PS1");
	char *user = getenv("USER");
	char *pwd = getenv("PWD");

	if (ps1) {
		printf("%s", ps1);
		return;
	}

	if (user)
		printf("%s ", user);
	if (pwd) {
		char *directory;
		if (strcmp(pwd, "/")) {
			while (*pwd)
				if (*(pwd++) == '/')
					directory = pwd;
		} else
			directory = pwd;
		printf("%s ", directory);
	}

	if (user)
		printf("%s ", strcmp(user, "root") ? "$" : "#");
	else
		printf("%s ", "$");
}

void print_prompt2(void)
{
	char *ps2 = getenv("PS2");

	if (ps2) {
		printf("%s", ps2);
		return;
	}

	printf("%s ", ">");
}

void print_set()
{
	char **envp = global_envp;
	if (!envp)
		return;

	while (*envp)
		printf("%s\n", *(envp++));
}
*/

int main(int argc, char **argv, char **envp)
{
	lexer_info lexer_info;
    init_lexer (&lexer_info);
/*	signal(SIGINT, SIG_IGN);  TODO: not ignore, continue main do..while */

/*	init_shell(envp);*/

	do {
/*        if (lexer_info.state == ST_EOLN_EOF)
            init_lexer(&lexer_info);*/
        lexeme *lex = get_lex (&lexer_info);
        printf ("!!! Lexeme: ");
        switch (lex->type) {
        case LEX_INPUT:         /* '<'  */
            printf ("[<]\n");
            break;
        case LEX_OUTPUT:        /* '>'  */
            printf ("[>]\n");
            break;
        case LEX_APPEND:        /* '>>' */
            printf ("[>>]\n");
            break;
        case LEX_PIPE:          /* '|'  */
            printf ("[|]\n");
            break;
        case LEX_OR:            /* '||' */
            printf ("[||]\n");
            break;
        case LEX_BACKGROUND:    /* '&'  */
            printf ("[&]\n");
            break;
        case LEX_AND:           /* '&&' */
            printf ("[&&]\n");
            break;
        case LEX_SEMICOLON:     /* ';'  */
            printf ("[;]\n");
            break;
        case LEX_BRACKET_OPEN:  /* '('  */
            printf ("[(]\n");
            break;
        case LEX_BRACKET_CLOSE: /* ')'  */
            printf ("[)]\n");
            break;
        case LEX_REVERSE:       /* '`'  */
            printf ("[`]\n");
            break;
        case LEX_WORD:     /* all different */
            printf ("[WORD:%s]\n", lex->str);
            break;
        case LEX_EOLINE:        /* '\n' */
            printf ("[EOLINE]\n");
            break;
        case LEX_EOFILE:         /* EOF  */
            printf ("[EOFILE]\n");
            break;
        }
        if (lex->type == LEX_EOFILE)
            return 0;

		if (lexer_info.state == ST_ERROR) {
			printf("(>_<)\n");
			continue;
		}
/*
#ifdef PRINT_PARSED_COMMAND
		printf("PARSED: ");
		print_command(cmd);
		printf("\n");
#endif
*/
/*		signal(SIGINT, SIG_DFL);  TODO: kill fg process */
/*		run_command(cmd); */
/*		signal(SIGINT, SIG_IGN);*/

/*		dispose_command(cmd); */
	} while (1);

	return 0;
}
