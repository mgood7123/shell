#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "word_buffer.h"

#ifdef PARSER_DEBUG
void parser_print_action(parser_info *pinfo, const char *where, int leaving)
{
    if (leaving)
        fprintf(stderr, "Parser: leaving from %s; ", where);
    else
        fprintf(stderr, "Parser: entering to %s; ", where);

    if (pinfo->cur_lex != NULL)
        print_lex(stderr, pinfo->cur_lex);
    else
        fprintf(stderr, "\n");
}
#endif

void parser_print_error(parser_info *pinfo, const char *where)
{
    fprintf(stderr, "Parser: error #%d in %s; ", pinfo->error, where);

    if (pinfo->cur_lex != NULL)
        print_lex(stderr, pinfo->cur_lex);
    else
        fprintf(stderr, "\n");
}

void parser_get_lex(parser_info *pinfo);

void init_parser(parser_info *pinfo)
{
/*  parser_info *pinfo =
        (parser_info *) malloc(sizeof(parser_info)); */

    pinfo->linfo = (lexer_info *) malloc(sizeof(lexer_info));
    init_lexer(pinfo->linfo);
    pinfo->cur_lex = NULL;
    pinfo->save_str = 0;
}

void parser_get_lex(parser_info *pinfo)
{
    if (pinfo->cur_lex != NULL) {
        if (pinfo->save_str)
            free(pinfo->cur_lex);
        else
            destroy_lex(pinfo->cur_lex);
    }

    pinfo->save_str = 0;
    pinfo->cur_lex = get_lex(pinfo->linfo);

#ifdef PARSER_DEBUG
    fprintf(stderr, "Get lex: ");
    print_lex(stderr, pinfo->cur_lex);
#endif

    if (pinfo->cur_lex->type == LEX_ERROR) /* Error 1 */
        pinfo->error = 1;
    else
        pinfo->error = 0; /* Error 0: all right */
}

cmd_pipeline_item *make_cmd_pipeline_item()
{
    cmd_pipeline_item *simple_cmd =
        (cmd_pipeline_item *) malloc(sizeof(cmd_pipeline_item));
    simple_cmd->argv = NULL;
    simple_cmd->input = NULL;
    simple_cmd->output = NULL;
    simple_cmd->append = 0;
    simple_cmd->cmd_lst = NULL;
    simple_cmd->next = NULL;
    return simple_cmd;
}

cmd_pipeline *make_cmd_pipeline()
{
    cmd_pipeline *pipeline =
        (cmd_pipeline *) malloc(sizeof(cmd_pipeline));
    pipeline->input = NULL;
    pipeline->output = NULL;
    pipeline->append = 0;
    pipeline->first_item = NULL;
    return pipeline;
}

cmd_list_item *make_cmd_list_item()
{
    cmd_list_item *list_item =
        (cmd_list_item *) malloc(sizeof(cmd_list_item));
    list_item->pl = NULL;
    list_item->rel = REL_NONE;
    list_item->next = NULL;
    return list_item;
}

cmd_list *make_cmd_list()
{
    cmd_list *list =
        (cmd_list *) malloc(sizeof(cmd_list));
    list->foreground = 1;
    list->first_item = NULL;
    return list;
}

void print_cmd_list(FILE *stream, cmd_list *list, int newline);

void print_cmd_pipeline(FILE *stream, cmd_pipeline *pipeline)
{
    cmd_pipeline_item *current;

    if (pipeline == NULL) {
        fprintf(stream, "NULL_PIPELINE");
        return;
    }

    current = pipeline->first_item;
    while (current != NULL) {
        if (current->cmd_lst == NULL) {
            print_argv(stream, current->argv);
        } else {
            fprintf(stream, "(");
            print_cmd_list(stream, current->cmd_lst, 0);
            fprintf(stream, ")");
        }
        if (current->next != NULL)
            fprintf(stream, " | ");
        current = current->next;
    }

    if (pipeline->input != NULL) {
        fprintf(stream, " < [%s]", pipeline->input);
    }
    if (pipeline->output != NULL) {
        if (pipeline->append)
            fprintf(stream, " >> [%s]", pipeline->output);
        else
            fprintf(stream, " > [%s]", pipeline->output);
    }
}

void print_relation(FILE *stream, type_of_relation rel) {
    switch (rel) {
    case REL_OR:
        fprintf(stream, " || ");
        break;
    case REL_AND:
        fprintf(stream, " && ");
        break;
    case REL_BOTH:
        fprintf(stream, " ; ");
        break;
    case REL_NONE:
        /* Do nothing */
        break;
    }
}

void print_cmd_list(FILE *stream, cmd_list *list, int newline)
{
    cmd_list_item *current;

    if (list == NULL) {
        fprintf(stream, "[NULL_CMD_LIST]\n");
        return;
    }

    current = list->first_item;
    while (current != NULL) {
        print_cmd_pipeline(stream, current->pl);
        print_relation(stream, current->rel);
        current = current->next;
    }

    if (!list->foreground)
        fprintf(stream, "&");
    if (newline)
        fprintf(stream, "\n");
}

void destroy_cmd_list(cmd_list *list);

void destroy_cmd_pipeline(cmd_pipeline *pipeline)
{
    cmd_pipeline_item *current;
    cmd_pipeline_item *next;

    if (pipeline == NULL)
        return;

    current = pipeline->first_item;
    while (current != NULL) {
        next = current->next;
        destroy_argv(current->argv);
        destroy_cmd_list(current->cmd_lst);
        free(current);
        current = next;
    }

    if (pipeline->input != NULL)
        free(pipeline->input);
    if (pipeline->output != NULL)
        free(pipeline->output);
    free(pipeline);
}

void destroy_cmd_list(cmd_list *list)
{
    cmd_list_item *current;
    cmd_list_item *next;

    if (list == NULL)
        return;

    current = list->first_item;
    while (current != NULL) {
        next = current->next;
        destroy_cmd_pipeline(current->pl);
        free(current);
        current = next;
    }

    free(list);
}

cmd_pipeline_item *parse_cmd_pipeline_item(parser_info *pinfo)
{
    cmd_pipeline_item *simple_cmd = make_cmd_pipeline_item();
    word_buffer wbuf;
    new_word_buffer(&wbuf);

#ifdef PARSER_DEBUG
    parser_print_action(pinfo, "parse_cmd_pipeline_item()", 0);
#endif

    do {
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            /* Add to word buffer for making argv */
            add_to_word_buffer(&wbuf, pinfo->cur_lex->str);
            pinfo->save_str = 1;
            parser_get_lex(pinfo);
            break;
        case LEX_INPUT:
            pinfo->error = (simple_cmd->input == NULL) ? 0 : 10; /* Error 10 */
            if (pinfo->error)
                goto error;

            parser_get_lex(pinfo);
            /* Lexer error possible */
            if (pinfo->error)
                goto error;

            pinfo->error = (pinfo->cur_lex->type != LEX_WORD) ? 12 : 0; /* Error 12 */
            if (pinfo->error)
                goto error;

            simple_cmd->input = pinfo->cur_lex->str;
            pinfo->save_str = 1;
            parser_get_lex(pinfo);
            break;
        case LEX_OUTPUT:
        case LEX_APPEND:
            pinfo->error = (simple_cmd->output == NULL) ? 0 : 11; /* Error 11 */
            if (pinfo->error)
                goto error;

            simple_cmd->append = (pinfo->cur_lex->type == LEX_OUTPUT) ? 0 : 1;
            parser_get_lex(pinfo);
            /* Lexer error possible */
            if (pinfo->error)
                goto error;

            pinfo->error = (pinfo->cur_lex->type != LEX_WORD) ? 2 : 0; /* Error 2 */
            if (pinfo->error)
                goto error;

            simple_cmd->output = pinfo->cur_lex->str;
            pinfo->save_str = 1;
            parser_get_lex(pinfo);
            break;
        default:
            /* Lexer error possible */
            if (pinfo->error)
                goto error;

            pinfo->error = (wbuf.count_words == 0) ? 3 : 0; /* Error 3 */
            if (pinfo->error)
                goto error;

            /* make argv from word buffer */
            simple_cmd->argv = convert_to_argv(&wbuf, 1);
#ifdef PARSER_DEBUG
            parser_print_action(pinfo, "parse_cmd_pipeline_item()", 1);
#endif
            return simple_cmd;
        }
    } while (!pinfo->error);

error:
#ifdef PARSER_DEBUG
    parser_print_error(pinfo, "parse_cmd_pipeline_item()");
#endif
    clear_word_buffer(&wbuf, 1);
    if (simple_cmd->input != NULL)
        free(simple_cmd->input);
    if (simple_cmd->output != NULL)
        free(simple_cmd->output);
    free(simple_cmd);
    return NULL;
}

cmd_list *parse_cmd_list(parser_info *pinfo);

cmd_pipeline *parse_cmd_pipeline(parser_info *pinfo)
{
    cmd_pipeline *pipeline = make_cmd_pipeline();
    cmd_pipeline_item *cur_item = NULL, *tmp_item = NULL;

#ifdef PARSER_DEBUG
    parser_print_action(pinfo, "parse_cmd_pipeline()", 0);
#endif

    do {
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            tmp_item = parse_cmd_pipeline_item(pinfo);
            break;
        case LEX_BRACKET_OPEN:
            tmp_item = make_cmd_pipeline_item();
            tmp_item->cmd_lst = parse_cmd_list(pinfo);
            if (pinfo->error) {
                free(tmp_item);
                goto error;
            }

            parser_get_lex(pinfo);
            break;
        default:
            pinfo->error = 4; /* Error 4 */
            break;
        }

        if (pinfo->error)
            goto error;

        if (pipeline->first_item == NULL) {
            /* First simple cmd */
            pipeline->first_item = cur_item = tmp_item;
            pipeline->input = cur_item->input;
            cur_item->input = NULL;
        } else {
            /* Second and following simple cmd */
            cur_item = cur_item->next = tmp_item;
            pinfo->error = (cur_item->input == NULL) ? 0 : 8; /* Error 8 */
            if (pinfo->error) {
                free(cur_item->input);
                if (cur_item->output != NULL)
                    free(cur_item->output);
                goto error;
            }
        }

        if (pinfo->cur_lex->type == LEX_PIPE) {
            /* Not last simple cmd */
            pinfo->error = (cur_item->output == NULL) ? 0 : 9; /* Error 9 */
            if (pinfo->error) {
                free(cur_item->output);
                if (cur_item->input != NULL)
                    free(cur_item->input);
                goto error;
            }

            parser_get_lex(pinfo);
            continue;
        } else {
            /* Last simple cmd in this pipeline */
            pipeline->output = cur_item->output;
            pipeline->append = cur_item->append;
            cur_item->output = NULL;
            cur_item->append = 0;
#ifdef PARSER_DEBUG
            parser_print_action(pinfo, "parse_cmd_pipeline()", 1);
#endif
            return pipeline;
        }
    } while (!pinfo->error);

error:
#ifdef PARSER_DEBUG
    parser_print_error(pinfo, "parse_cmd_pipeline()");
#endif
    destroy_cmd_pipeline(pipeline);
    return NULL;
}

cmd_list_item *parse_cmd_list_item(parser_info *pinfo)
{
    cmd_list_item *list_item = make_cmd_list_item(); /* marked */

#ifdef PARSER_DEBUG
    parser_print_action(pinfo, "parse_cmd_list_item()", 0);
#endif

    list_item->pl = parse_cmd_pipeline(pinfo);
    if (pinfo->error)
        goto error;

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
        parser_get_lex(pinfo);

    if (pinfo->error)
        goto error;

#ifdef PARSER_DEBUG
    parser_print_action(pinfo, "parse_cmd_list_item()", 1);
#endif
    return list_item;

error:
#ifdef PARSER_DEBUG
    parser_print_error(pinfo, "parse_cmd_list_item()");
#endif
    free(list_item);
    return NULL;
}

cmd_list *parse_cmd_list(parser_info *pinfo)
{
    int bracket_terminated = 0;
    int lex_term = 0;
    cmd_list *list = make_cmd_list(pinfo);
    cmd_list_item *cur_item = NULL, *tmp_item = NULL;

#ifdef PARSER_DEBUG
    parser_print_action(pinfo, "parse_cmd_list()", 0);
#endif

    if (pinfo->cur_lex != NULL)
        bracket_terminated = (pinfo->cur_lex->type == LEX_BRACKET_OPEN);

    parser_get_lex(pinfo);
    if (pinfo->error)
        goto error;

    switch (pinfo->cur_lex->type) {
    case LEX_EOLINE:
    case LEX_EOFILE:
        /* Empty cmd list */
        pinfo->error = 16; /* Error 16 */
        goto error;
    default:
        /* Do nothing */
        break;
    }

    do {
        tmp_item = parse_cmd_list_item(pinfo); /* marked */
        if (pinfo->error)
            goto error;

        if (list->first_item == NULL)
            list->first_item = cur_item = tmp_item;
        else
            cur_item = cur_item->next = tmp_item;

        switch (pinfo->cur_lex->type) {
        case LEX_BACKGROUND:
        case LEX_BRACKET_CLOSE:
        case LEX_EOLINE:
        case LEX_EOFILE:
            lex_term = 1;
            break;
        default:
            pinfo->error = (cur_item->rel == REL_NONE) ? 15 : 0; /* Error 15 */
            break;
        }
    } while (!lex_term && !pinfo->error);

    if (pinfo->error)
        goto error;

    if (pinfo->cur_lex->type == LEX_BACKGROUND) {
        pinfo->error = (cur_item->rel == REL_NONE) ? 0 : 13; /* Error 13 */
        if (pinfo->error)
            goto error;
        list->foreground = 0;
        parser_get_lex(pinfo);
    }

    if (pinfo->error)
        goto error;

    switch (pinfo->cur_lex->type) {
    case LEX_BRACKET_CLOSE:
        pinfo->error = (bracket_terminated) ? 0 : 5; /* Error 5 */
        break;
    case LEX_EOLINE:
    case LEX_EOFILE:
        pinfo->error = (bracket_terminated) ? 6 : 0; /* Error 6 */
        break;
    default:
        pinfo->error = 14; /* Error 14 */
        break;
    }

    if (pinfo->error)
        goto error;

    pinfo->error = ((cur_item->rel == REL_NONE)
        || (cur_item->rel == REL_BOTH)) ? 0 : 7; /* Error 7 */
    if (pinfo->error)
        goto error;

    cur_item->rel = REL_NONE;

#ifdef PARSER_DEBUG
    parser_print_action(pinfo, "parse_cmd_list()", 1);
#endif
    return list;

error:
#ifdef PARSER_DEBUG
    parser_print_error(pinfo, "parse_cmd_list()");
#endif
    destroy_cmd_list(list);
    return NULL;
}

/* Compile:
gcc -g -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g -Wall -ansi -pedantic -c lexer.c -o lexer.o &&
gcc -g -Wall -ansi -pedantic -c word_buffer.c -o word_buffer.o &&
gcc -g -Wall -ansi -pedantic parser.c buffer.o lexer.o word_buffer.o utils.o -o parser
 * Grep possible parsing errors:
grep -Pn '\* Error \d+ \*' parser.c
*/

#ifdef PARSER
int main()
{
    cmd_list *list;
    parser_info pinfo;
    init_parser(&pinfo);

    do {
        list = parse_cmd_list(&pinfo); /* marked */

        switch (pinfo.error) {
        case 0:
            print_cmd_list(stdout, list, 1); /* marked */
            destroy_cmd_list(list);
            list = NULL;
            break;
        case 16:
            fprintf(stderr, "Parser: empty command;\n");
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
#endif
