#include "word_buffer.h"
#include "lexer.h"
#include <stdlib.h>

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
    struct cmd_pipeline *pl;
/*    struct cmd_list *cmd_lst;*/
    type_of_relation rel;
    /* End of item data */
    struct cmd_list_item *next;
} cmd_list_item;

typedef struct cmd_list {
    unsigned int foreground:1;
    struct cmd_list_item *first_item;
} cmd_list;

typedef struct parser_info {
    lexer_info *linfo;
    lexeme *cur_lex;
} parser_info;

void parser_get_lex (parser_info *pinfo);

void init_parser (parser_info *pinfo)
{
/*  parser_info *pinfo =
        (parser_info *) malloc (sizeof (parser_info)); */


    pinfo->linfo = (lexer_info *) malloc (sizeof (lexer_info));
    init_lexer (pinfo->linfo);
    pinfo->cur_lex = NULL;
    parser_get_lex (pinfo);
}

void parser_get_lex (parser_info *pinfo)
{
    if (pinfo->cur_lex != NULL)
        free (pinfo->cur_lex); /* do not freeing string */
    pinfo->cur_lex = get_lex (pinfo->linfo);
}

cmd_pipeline_item *make_cmd_pipeline_item ()
{
    cmd_pipeline_item *simple_cmd =
        (cmd_pipeline_item *) malloc (sizeof (cmd_pipeline_item));
    simple_cmd->argv = NULL;
    simple_cmd->input = NULL;
    simple_cmd->output = NULL;
    simple_cmd->append = 0;
    simple_cmd->cmd_lst = NULL;
    simple_cmd->next = NULL;
    return simple_cmd;
}

cmd_pipeline *make_cmd_pipeline ()
{
    cmd_pipeline *pipeline =
        (cmd_pipeline *) malloc (sizeof (cmd_pipeline));
    pipeline->input = NULL;
    pipeline->output = NULL;
    pipeline->append = 0;
    pipeline->first_item = NULL;
    return pipeline;
}

cmd_list_item *make_cmd_list_item ()
{
    cmd_list_item *list_item =
        (cmd_list_item *) malloc (sizeof (cmd_list_item));
    list_item->pl = NULL;
    list_item->rel = REL_BOTH;
    list_item->next = NULL;
    return list_item;
}

cmd_list *make_cmd_list ()
{
    cmd_list *list =
        (cmd_list *) malloc (sizeof (cmd_list));
    list->foreground = 1;
    list->first_item = NULL;
    return list;
}

/* TODO
 * 1. Проверки (input и output), один ли раз было перенаправление
 * этого типа.
 * 2. Подумать над редиректами до имени команды. */
cmd_pipeline_item *parse_cmd_pipeline_item (parser_info *pinfo)
{
    cmd_pipeline_item *simple_cmd = make_cmd_pipeline_item ();
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
                    simple_cmd->input = pinfo->cur_lex->str;
                    break;
                case LEX_OUTPUT:
                    simple_cmd->output = pinfo->cur_lex->str;
                    simple_cmd->append = 0;
                    break;
                case LEX_APPEND:
                    simple_cmd->output = pinfo->cur_lex->str;
                    simple_cmd->append = 1;
                    break;
                default:
                    /* Not possible */
                    break;
            }
            parser_get_lex (pinfo);
            break;
        default:
            /* make argv from word buffer */
            simple_cmd->argv = convert_to_argv (&wbuf, 1);
            return simple_cmd;
        }
    } while (1);
}

cmd_list *parse_cmd_list (parser_info *pinfo);

/* TODO:
 * 1. Обработка ошибок парсинга item'ов
 * 2. ловить некорректные редирректы
 * Корректные: на вход в первом simple cmd, на выход в последнем.
 */
cmd_pipeline *parse_cmd_pipeline (parser_info *pinfo)
{
    cmd_pipeline *pipeline = make_cmd_pipeline ();
    cmd_pipeline_item *cur_item = NULL, *tmp_item = NULL;

    do {
        if (pinfo->cur_lex->type == LEX_WORD) {
            tmp_item = parse_cmd_pipeline_item (pinfo);
        } else if (pinfo->cur_lex->type == LEX_BRACKET_OPEN) {
            parser_get_lex (pinfo);
            tmp_item = make_cmd_pipeline_item ();
            cur_item->cmd_lst = parse_cmd_list (pinfo);
            if (pinfo->cur_lex->type == LEX_BRACKET_CLOSE) {
                parser_get_lex (pinfo);
            } else {
                /* TODO: error */
            }
        } else {
            /* TODO: error */
        }

        /* TODO: processing errors */

        if (pipeline->first_item == NULL) {
            /* First simple cmd */
            pipeline->first_item = cur_item = tmp_item;
        } else {
            cur_item = cur_item->next = tmp_item;
        }

        if (pinfo->cur_lex->type == LEX_PIPE) {
            parser_get_lex (pinfo);
            continue;
        } else {
            return pipeline;
        }
    } while (1);
}

cmd_list_item *parse_cmd_list_item (parser_info *pinfo)
{
    cmd_list_item *list_item = make_cmd_list_item ();
    list_item->pl = parse_cmd_pipeline (pinfo);
    /* TODO: processing errors */
    switch (pinfo->cur_lex->type) {
    case LEX_OR:
        list_item->rel = REL_OR;
        break;
    case LEX_AND:
        list_item->rel = REL_AND;
        break;
    case LEX_SEMICOLON:
        list_item->rel = REL_BOTH;
        break;
    default:
        /* TODO: error */
        free (list_item);
        return NULL;
    }

    parser_get_lex (pinfo);
    return list_item;
}

cmd_list *parse_cmd_list (parser_info *pinfo)
{
    cmd_list *list = make_cmd_list (pinfo);
    cmd_list_item *cur_item = NULL, *tmp_item = NULL;

    do {
        tmp_item = parse_cmd_list_item (pinfo);
        /* TODO: error */
        if (list->first_item == NULL)
            list->first_item = cur_item = tmp_item;
        else
            cur_item = cur_item->next = tmp_item;

        if (pinfo->cur_lex->type == LEX_BACKGROUND) {
            list->foreground = 0;
            parser_get_lex (pinfo);
        }

        switch (pinfo->cur_lex->type) {
        case LEX_BRACKET_CLOSE:
        case LEX_EOLINE:
        case LEX_EOFILE:
            return list;
        default:
            /* TODO: error */
            /* frees */
            return NULL;
        }
    } while (1);
}

/*
gcc -g -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g -Wall -ansi -pedantic -c lexer.c -o lexer.o &&
gcc -g -Wall -ansi -pedantic -c word_buffer.c -o word_buffer.o &&
gcc -g -Wall -ansi -pedantic parser.c buffer.o lexer.o word_buffer.o -o parser
*/

int main ()
{
    cmd_list *list;
    parser_info pinfo;
    init_parser (&pinfo);
    list = parse_cmd_list (&pinfo);
    return 0;
}
