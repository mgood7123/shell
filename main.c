#include "main.h"

/* Shell initialization:
 * save envp;
 * set PWD enviroment variable;
 * change umask;
 * ignore some signals;
 * get shell group id
 * put shell group to foreground */

/* Note: this is not check for
 * interactively/background runned shell. */
void init_shell(shell_info *sinfo, char **envp)
{
    char *cur_dir = getcwd(NULL, 0);
    if (cur_dir != NULL) {
        setenv("PWD", cur_dir, 1);
        free(cur_dir);
    }

    umask(S_IWGRP | S_IWOTH);

    new_shell_info(sinfo);
    sinfo->envp = envp;
    sinfo->shell_pgid = getpid();
    sinfo->shell_interactive = isatty(sinfo->orig_stdin);

    if (sinfo->shell_interactive) {
        set_sig_ign();
        if (TCSETPGRP_ERROR (
                tcsetpgrp(sinfo->orig_stdin, sinfo->shell_pgid)))
        {
            exit(ES_SYSCALL_FAILED);
        }
    }
}

int main(int argc, char **argv, char **envp)
{
    shell_info sinfo;
    cmd_list *list;
    parser_info pinfo;

    init_shell(&sinfo, envp);
    init_parser(&pinfo);

    do {
        update_jobs_status(&sinfo);
        print_prompt1();
        list = parse_cmd_list(&pinfo);

        switch (pinfo.error) {
        case 0:
#if 1
            run_cmd_list(&sinfo, list);
#else
            print_cmd_list(stdout, list, 1);
            destroy_cmd_list(list);
#endif
            list = NULL;
            break;
        case 16:
#ifdef PARSER_DEBUG
            fprintf(stderr, "Parser: empty command;\n");
#endif
            /* Empty command is not error */
            pinfo.error = 0;
            break;
        default:
            /* TODO: flush read buffer,
             * possibly via buffer function. */
            fprintf(stderr, "Parser: bad command;\n");
            break;
        }

        if (pinfo.cur_lex->type == LEX_EOFILE)
            exit(pinfo.error);
    } while (1);

    return 0;
}
