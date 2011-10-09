#include "word_buffer.h"
#include "lexer.h"
#include <stdlib.h>

typedef struct cmd_pipeline_item {
    /* Item data: */
    char **argv;
    char *input;  /* temporally */
    char *output; /* temporally */
    unsigned int append:1; /* temporally */
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
    REL_NONE,  /* no relation */
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

#define PARSER_DEBUG

#ifdef PARSER_DEBUG
#include <stdio.h>
#endif

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
    list_item->rel = REL_NONE;
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

void print_cmd_list (FILE *stream, cmd_list *list);

void print_cmd_pipeline (FILE *stream, cmd_pipeline *pipeline)
{
    cmd_pipeline_item *current;

    if (pipeline == NULL) {
        fprintf (stream, "NULL_PIPELINE");
        return;
    }

    current = pipeline->first_item;
    while (current != NULL) {
        if (current->cmd_lst == NULL) {
            print_argv (stream, current->argv);
        } else {
            fprintf (stream, "(");
            print_cmd_list (stream, current->cmd_lst);
            fprintf (stream, ")");
        }
        if (current->next != NULL)
            fprintf (stream, " | ");
        current = current->next;
    }

    if (pipeline->input != NULL) {
        fprintf (stream, " < [%s]", pipeline->input);
    }
    if (pipeline->output != NULL) {
        if (pipeline->append)
            fprintf (stream, " >> [%s]", pipeline->output);
        else
            fprintf (stream, " > [%s]", pipeline->output);
    }
}

void print_relation (FILE *stream, type_of_relation rel) {
    switch (rel) {
    case REL_OR:
        fprintf (stream, " || ");
        break;
    case REL_AND:
        fprintf (stream, " && ");
        break;
    case REL_BOTH:
        fprintf (stream, " ; ");
        break;
    case REL_NONE:
        /* No actions */
        break;
    }
}

void print_cmd_list (FILE *stream, cmd_list *list)
{
    cmd_list_item *current;

    if (list == NULL) {
        fprintf (stream, "[NULL_CMD_LIST]\n");
        return;
    }

    current = list->first_item;
    while (current != NULL) {
        print_cmd_pipeline (stream, current->pl);
        print_relation (stream, current->rel);
        current = current->next;
    }

    if (!list->foreground)
        fprintf (stream, "&");
}

void destroy_cmd_list (cmd_list *list);

void destroy_cmd_pipeline (cmd_pipeline *pipeline)
{
    cmd_pipeline_item *current;
    cmd_pipeline_item *next;

    if (pipeline == NULL)
        return;

    current = pipeline->first_item;
    while (current != NULL) {
        next = current->next;
        free (current->argv);
        destroy_cmd_list (current->cmd_lst);
        free (current);
        current = next;
    }

    if (pipeline->input != NULL)
        free (current->input);
    if (pipeline->output != NULL)
        free (current->output);
    free (pipeline);
}

void destroy_cmd_list (cmd_list *list)
{
    cmd_list_item *current;
    cmd_list_item *next;

    if (list == NULL)
        return;

    current = list->first_item;
    while (current != NULL) {
        next = current->next;
        destroy_cmd_pipeline (current->pl);
        free (current);
        current = next;
    }

    free (list);
}

/* TODO
 * 1. Проверки (input и output), один ли раз было перенаправление
 * этого типа.
 * 2. Подумать над редиректами до имени команды. */
cmd_pipeline_item *parse_cmd_pipeline_item (parser_info *pinfo)
{
    int error = (pinfo->cur_lex->type == LEX_ERROR) ? 1 : 0;
    cmd_pipeline_item *simple_cmd = make_cmd_pipeline_item ();
    word_buffer wbuf;
    type_of_lex redirect_type; /* temporally */
    new_word_buffer (&wbuf);

#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: entering to parse_cmd_pipeline_item (); ");
    print_lex (stderr, pinfo->cur_lex);
#endif

    while (!error) {
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            /* Add to word buffer for making argv */
            add_to_word_buffer (&wbuf, pinfo->cur_lex->str);
            parser_get_lex (pinfo);
            break;
        case LEX_INPUT:
        case LEX_OUTPUT:
        case LEX_APPEND:
            redirect_type = pinfo->cur_lex->type;
            parser_get_lex (pinfo);
            error = (pinfo->cur_lex->type != LEX_WORD) ? 2 : 0;
            if (error)
                continue;
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
            error = (pinfo->cur_lex->type == LEX_ERROR) ? 3 : 0;
            if (error)
                continue;
            error = (wbuf.count_words == 0) ? 4 : 0;
            if (error)
                continue;
            /* make argv from word buffer */
            simple_cmd->argv = convert_to_argv (&wbuf, 1);
#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: leaving from parse_cmd_pipeline_item (); ");
    print_lex (stderr, pinfo->cur_lex);
#endif
            return simple_cmd;
        }
    }

    /* Error processing */
#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: error #%d in parse_cmd_pipeline_item (); ", error);
    print_lex (stderr, pinfo->cur_lex);
#endif
    clear_word_buffer (&wbuf, 1);
    free (simple_cmd);
    return NULL;
}

cmd_list *parse_cmd_list (parser_info *pinfo, int bracket_terminated);

/* TODO: clear input/output pointers in cmd_pipeline_item
 * at parsing cmd_pipeline */
/* Ловить точку с запятой в конце, менять тип на REL_NONE */
/* TODO:
 * ловить некорректные редирректы
 * Корректные: на вход в первом simple cmd, на выход в последнем.
 */
cmd_pipeline *parse_cmd_pipeline (parser_info *pinfo)
{
    int error = (pinfo->cur_lex->type == LEX_ERROR) ? 1 : 0;
    cmd_pipeline *pipeline = make_cmd_pipeline ();
    cmd_pipeline_item *cur_item = NULL, *tmp_item = NULL;

#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: entering to parse_cmd_pipeline (); ");
    print_lex (stderr, pinfo->cur_lex);
#endif

    while (!error) {
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            tmp_item = parse_cmd_pipeline_item (pinfo);
            error = (tmp_item == NULL) ? 2 : 0;
            break;
        case LEX_BRACKET_OPEN:
            /* TODO: переместить считывание
             * открывающей скобки в parse_cmd_list */
            parser_get_lex (pinfo);
            tmp_item = make_cmd_pipeline_item ();
            tmp_item->cmd_lst = parse_cmd_list (pinfo, 1);
            error = (tmp_item->cmd_lst == NULL) ? 3 : 0;
            break;
        default:
            error = 4;
            break;
        }

        if (error)
            continue;

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
#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: leaving from parse_cmd_pipeline (); ");
    print_lex (stderr, pinfo->cur_lex);
#endif
            return pipeline;
        }
    }

    /* Error processing */
#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: error #%d in parse_cmd_pipeline (); ", error);
    print_lex (stderr, pinfo->cur_lex);
#endif
    destroy_cmd_pipeline (pipeline);
    return NULL;
}

cmd_list_item *parse_cmd_list_item (parser_info *pinfo)
{
    cmd_list_item *list_item = make_cmd_list_item ();

#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: entering to parse_cmd_list_item (); ");
    print_lex (stderr, pinfo->cur_lex);
#endif

    list_item->pl = parse_cmd_pipeline (pinfo);
    if (list_item->pl == NULL) {
        /* Error processing */
#ifdef PARSER_DEBUG
        fprintf (stderr, "Parser: error in parse_cmd_list_item (); ");
        print_lex (stderr, pinfo->cur_lex);
#endif
        free (list_item);
        return NULL;
    }

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
        list_item->rel = REL_NONE;
        break;
    }

    if (list_item->rel != REL_NONE)
        parser_get_lex (pinfo);

#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: leaving from to parse_cmd_list_item (); ");
    print_lex (stderr, pinfo->cur_lex);
#endif
    return list_item;
}

cmd_list *parse_cmd_list (parser_info *pinfo, int bracket_terminated)
{
    int error = (pinfo->cur_lex->type == LEX_ERROR) ? 1 : 0;
    cmd_list *list = make_cmd_list (pinfo);
    cmd_list_item *cur_item = NULL, *tmp_item = NULL;

#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: entering to parse_cmd_list (pinfo, %d); ",
        bracket_terminated);
    print_lex (stderr, pinfo->cur_lex);
#endif

    while (!error) {
        tmp_item = parse_cmd_list_item (pinfo);
        error = (tmp_item == NULL) ? 2 : 0;
        if (error)
            continue;
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
            error = (bracket_terminated) ? 0 : 3;
            if (error)
                continue;
            break;
        case LEX_EOLINE:
        case LEX_EOFILE:
            error = (bracket_terminated) ? 4 : 0;
            if (error)
                continue;
            break;
        default:
            /* No actions */
            continue;
        }

        if (!error) {
            parser_get_lex (pinfo);
#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: leaving from parse_cmd_list (pinfo, %d); ",
        bracket_terminated);
    print_lex (stderr, pinfo->cur_lex);
#endif
            return list;
        }
    }

    /* Error processing */
#ifdef PARSER_DEBUG
    fprintf (stderr, "Parser: error #%d in parse_cmd_list (pinfo, %d); ",
        error, bracket_terminated);
    print_lex (stderr, pinfo->cur_lex);
#endif
    destroy_cmd_list (list);
    return NULL;
}

/*
gcc -g -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g -Wall -ansi -pedantic -c lexer.c -o lexer.o &&
gcc -g -Wall -ansi -pedantic -c word_buffer.c -o word_buffer.o &&
gcc -g -Wall -ansi -pedantic parser.c buffer.o lexer.o word_buffer.o -o parser
*/

#include <stdio.h>

int main ()
{
    cmd_list *list;
    parser_info pinfo;
    init_parser (&pinfo);
    do {
        list = parse_cmd_list (&pinfo, 0);
        if (list == NULL) {
            fprintf (stderr, "Parser: error;\n");
            return 1;
        }
        print_cmd_list (stdout, list);
    } while (1);

    return 0;
}
