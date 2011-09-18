#ifndef _LEXER_H
#define _LEXER_H

typedef enum type_of_lex {
    LEX_INPUT,         /* '<'  */
    LEX_OUTPUT,        /* '>'  */
    LEX_APPEND,        /* '>>' */
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
    LEX_EOFILE         /* EOF  */
} type_of_lex;

typedef struct lexeme {
    struct lexeme *next;
    type_of_lex type;
    char *str;
} lexeme;

typedef enum lexer_state {
    START,
    ONE_SYM_LEX,
    /* '<', ';', '(', ')' */
    ONE_TWO_SYM_LEX,
    /* '>', '>>', '|', '||', '&', '&&' */
    BACKSLASH,
    BACKSLASH_IN_QUOTES,
    IN_QUOTES,
    ERROR,
    OTHER,
    END_OF
} lexer_state;

typedef struct lexer_info {
    lexer_state state;
    char c; /* current symbol */
} lexer_info;

void init_lexer (lexer_info *info);
lexeme *get_lex(lexer_info *info);

#endif
