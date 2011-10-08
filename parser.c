#include "word_buffer.h"
#include "lexer.h"

typedef struct cmd_pipeline_item {
    /* Item data: */
    char **argv;
    char *input;
    char *output;
    unsigned int append:1;
    struct cmd_list *cmd_lst;
    /* End of item data */
    struct cmd_pipeline_item *next;
} cmd_pipeline_item;

typedef struct cmd_pipeline {
    char *input;
    char *output;
    unsigned int append:1;
    struct cmd_pipeline_item *first_item;
} cmd_pipeline;

typedef enum type_of_relation {
    REL_OR,    /* '||' */
    REL_AND,   /* '&&' */
    REL_BOTH   /* ';'  */
} type_of_relation;

typedef struct cmd_list_item {
    /* Item data: */
    struct cmd_pipeline *cmd_pl;
    struct cmd_list *cmd_lst;
    type_of_relation rel;
    /* End of item data */
    struct cmd_list_item *next;
} cmd_list_item;

typedef struct cmd_list {
    unsigned int foreground:1;
    struct cmd_list_item *first_item;
}

typedef struct parser_info {
    lexer_info *linfo;
    lexeme *cur_lex;
} parser_info;

void init_parser (parser_info *pinfo)
{
    /* Пусть это делает кто-то другой, в main ()
    pinfo->linfo = (lexer_info *) malloc (sizeof (lexer_info)); */
    init_lexer (pinfo->linfo);
    pinfo->cur_lex = get_lex (pinfo->linfo);
}

void parser_get_lex (parser_info *pinfo)
{
    if (pinfo->cur_lex != NULL)
        destroy_lex (pinfo->cur_lex)
    pinfo->cur_lex = get_lex (pinfo->linfo);
}

*cmd_pipeline_item make_simple_cmd ()
{
    cmd_pipeline_item *simple_cmd =
        (cmd_pipeline_item *) malloc (sizeof (cmd_pipeline_item));
    simple_cmd->argv = NULL;
    simple_cmd->input = NULL;
    simple_cmd->output = NULL;
    simple_cmd->append = 0;
    simple_cmd->cmd_lst = NULL;
    simple_cmd->next = NULL
    return simple_cmd;
}

/* TODO
 * 1. Проверки (input и output), один ли раз было перенаправление
 * этого типа. */
*cmd_pipeline_item parse_cmd_pipeline_item (parser_info *pinfo)
{
    cmd_pipeline_item *simple_cmd = make_simple_cmd ();
    word_buffer wbuf;
    type_of_lex redirect_type; /* temporally */
    new_word_buffer (&wbuf);

    do {
        /* TODO: processing lexer errors */
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            /* Add to word buffer for making argv */
            add_to_word_buffer (&wbuf, pinfo->cur_lex->str);
            break;
        case LEX_INPUT:
        case LEX_OUTPUT:
        case LEX_APPEND:
            redirect_type = pinfo->cur_lex->type;
            parser_get_lex (pinfo);
            if (pinfo->cur_lex->type != LEX_WORD) {
                /* TODO: Error */
                /* pinfo->cur_lex->str will be destroy
                 * in parser_get_lex () */
                clear_word_buffer (&wbuf, 0);
                return NULL;
            }
            switch (redirect_type) {
                case LEX_INPUT:
                    simple_cmd->input = cur_lex->str;
                    break;
                case LEX_OUTPUT:
                    simple_cmd->output = cur_lex->str;
                    simple_cmd->append = 0;
                    break;
                case LEX_APPEND:
                    simple_cmd->output = cur_lex->str;
                    simple_cmd->append = 1;
                    break;
            }
            parser_get_lex (pinfo);
            break;
        default:
            /* make argv from word buffer */
            simple_cmd->pinfo->argv = convert_to_argv (&wbuf, 1);
            return simple_cmd;
        }
    } while (1);
}

/* TODO:
 * ловить некорректные редирректы
 * Корректные: на вход в первом simple cmd, на выход в последнем.
 */
*cmd_pipeline parse_cmd_pipeline (parser_info *pinfo)
{
    cmd_pipeline_item *list = parse_cmd_pipeline_item (pinfo);
    cmd_pipeline_item *cur_item = list;

    do {
        switch (pinfo->cur_lex->type) {
        case LEX_PIPE:
            parser_get_lex (pinfo);
            /* Make new item */
            break;
        default:
            /* make argv from strlist */
            return list;
        }
    } while (1);
}

*cmd_list_item parse_cmd_list_item (parser_info *pinfo)
{
    parser_get_lex (pinfo->linfo);
    switch (pinfo->cur_lex->type) {
    case LEX_INPUT:         /* '<'  */
    case LEX_OUTPUT:        /* '>'  */
    case LEX_APPEND:        /* '>>' */
    case LEX_PIPE:          /* '|'  */
    case LEX_OR:            /* '||' */
    case LEX_BACKGROUND:    /* '&'  */
    case LEX_AND:           /* '&&' */
    case LEX_SEMICOLON:     /* ';'  */
    case LEX_BRACKET_CLOSE: /* ')'  */
        /* Error */
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
        break;
    case LEX_REVERSE:       /* '`'  */
        break;
    case LEX_WORD:     /* all different */
        /* todo: make pipeline */
        break;
    case LEX_EOLINE:        /* '\n' */
        break;
    case LEX_EOFILE:         /* EOF  */
        break;
    }
}

*cmd_list parse_cmd_list (parser_info *pinfo)
{
    /* TODO:
    malloc cmd_list;
    cmd_list->first_item = parse_cmd_list_item (pinfo);
    cmd_list->foregroung = 1;
    switch (pinfo->cur_lex->type) {
    case LEX_BACKGROUND:
        cmd_list->foreground = 0;
        parse_get_lex (pinfo);
        break;
    case LEX_SEMICOLON:
        parse_get_lex (pinfo);
        break;
    case LEX_BRACKET_CLOSE:
        * TODO: We begin with open bracket? *
        parse_get_lex (pinfo);
        break;
    default:
        * No actions *
        break;
    }
    */
}
