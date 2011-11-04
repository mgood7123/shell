#ifndef PARSER_H_SENTRY
#define PARSER_H_SENTRY

/* Not defined by default */
#if !defined(PARSER_DEBUG) && 0
#define PARSER_DEBUG
#endif

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
    type_of_relation rel;
    /* End of item data */
    struct cmd_list_item *next;
} cmd_list_item;

typedef struct cmd_list {
    unsigned int foreground:1;
    struct cmd_list_item *first_item;
} cmd_list;

#include "lexer.h"
typedef struct parser_info {
    lexer_info *linfo;
    lexeme *cur_lex;
    int error;
    unsigned int save_str;
} parser_info;

void init_parser (parser_info *pinfo);
cmd_list *parse_cmd_list (parser_info *pinfo);
void destroy_cmd_list (cmd_list *list);

#include <stdio.h>
void print_cmd_list (FILE *stream, cmd_list *list, int newline);

#endif
