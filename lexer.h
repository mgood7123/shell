#pragma once
#ifndef LEXER_H_SENTRY
#define LEXER_H_SENTRY

/* Not defined by default */
/* 
#define LEXER_DEBUG
*/

#define ES_LEXER_INCURABLE_ERROR 1
#include <stdio.h>
#include "../CCR/Scripts/Shell/builtins/colors.h"
#include "../CCR/Scripts/Shell/builtins/printfmacro.h"
#include "../CCR/Scripts/Shell/builtins/env.h"
#include "../CCR/Scripts/Shell/builtins/regex_str.h"


typedef enum type_of_lex {
    LEX_INPUT,         /* '<'  */
    LEX_OUTPUT,        /* '>'  */
    LEX_APPEND,        /* '>>' */
    LEX_HEREDOC,       /* '<<' */
    LEX_HEREDOC_WORD,  /* '<<' */
    LEX_HERESTRING,    /* '<<<' */
    LEX_PIPE,          /* '|'  */
    LEX_OR,            /* '||' */
    LEX_BACKGROUND,    /* '&'  */
    LEX_AND,           /* '&&' */
    LEX_SEMICOLON,     /* ';'  */
    LEX_BRACKET_OPEN,  /* '('  */
    LEX_BRACKET_CLOSE, /* ')'  */
    LEX_REVERSE,       /* '`'  */
    LEX_WORD,     /* all different */
    LEX_EOLINE,        /* '\n' */
    LEX_EOFILE,        /* EOF  */
    LEX_ERROR     /* error in lexer */
} type_of_lex;

typedef struct lexeme {
/*    struct lexeme *next; */
    type_of_lex type;
    char *str;
} lexeme;

typedef enum lexer_state {
    ST_START,
    ST_ONE_SYM_LEX,
    /* ';', '(', ')' */
    ST_ONE_TWO_SYM_LEX,
    /* '>', '>>', '|', '||', '&', '&&', '$', '$$', '$@' */
    ST_ONE_TWO_THREE_SYM_LEX,
	/* '<', '<<', '<<<' */
    ST_BACKSLASH,
    ST_BACKSLASH_IN_QUOTES,
    ST_IN_QUOTES,
    ST_IN_HEREDOC,
    ST_ERROR,
    ST_WORD,
    ST_EOLN_EOF
} lexer_state;

typedef struct lexer_info {
    lexer_state state;
    int c; /* current symbol */
    unsigned int get_next_char:1;
} lexer_info;

void init_lexer(lexer_info *info);
lexeme *get_lex(lexer_info *info);
void destroy_lex(lexeme *lex);

void print_lex(FILE *stream, lexeme *lex);

struct heredoc {
	int pending;
	int accepting;
	int final;
	char * id;
	char last_char;
	char last_char_before_that;
	struct regex_string contents;
	int count;
} * heredoc = {0};
int heredoc_nest = 0;

struct lexer_tokens{
	env_t token;
	env_t token_column;
	env_t token_line;
	env_t token_type;
	int column;
	int line;
	int column_s;
	int line_s;
} lexer_tokens = {0};

char * token;
char * token_type;
int token_column;
int token_line;

char * last_token;
char * last_token_type;
int last_token_column;
int last_token_line;

char * last_token_before_that;
char * last_token_type_before_that;
int last_token_before_that_column;
int last_token_before_that_line;

#endif
