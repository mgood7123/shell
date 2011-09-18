/* TODO: move to buffer.c
#include "strlist.h"

#define BLOCK_SIZE 8
#define REAL_BLOCK_SIZE (BLOCK_SIZE+1)
*/

enum type_of_lex {
    LEX_INPUT;
    LEX_OUTPUT;
    LEX_APPEND;
    LEX_PIPE;
    LEX_OR;
    LEX_BACKGROUND;
    LEX_AND;
    LEX_SEMICOLON;
    LEX_BRACKET_OPEN;
    LEX_BRACKET_CLOSE;
    LEX_REVERSE;
    LEX_WORD;
};

typedef struct lex {
    struct lex *next;
    type_of_lex type;
    char *str;
} lex;

enum lexer_state {
    START;

    ONE_SYM_LEX;
    /* '<', ';', '(', ')' */
    ONE_TWO_SYM_LEX;
    /* '>', '>>', '|', '||', '&', '&&'; */

    BACKSLASH;
    BACKSLASH_IN_QUOTES;
    IN_QUOTES;

    ERROR;
    OTHER;
}

typedef struct lexer_info {
    lexer_state state;
    char c; /* current symbol */
    buffer buf; /* symbols buffer */
}

lex *make_lex (type_of_lex type)
{
    lex *lex = (lex *) malloc(sizeof(lex));
    lex->next = NULL;
    lex->type = type;
    lex->str = NULL;
    return lex;
}

void get_char (lexer_info *info);
void flush_buffer ();
void add (lexer_info *info, char sym);
void add_current (lexer_info *info);


void add_current(lexer_info *info)
{
    add(info, info->c);
}

lex *get_lex(lexer_info *info)
{
    do {
        switch (info->state) {
        case START:
            switch (info->c) {
            case EOF:
                /* TODO */
                break;
            case '\n':
                /* TODO */
                break;
            case ' ':
                get_char (info);
                break;
            case '<':
            case ';':
            case '(':
            case ')':
                get_char (info);
                info->state = ONE_SYM_LEX;
                break;
            case '>':
            case '|':
            case '&':
                get_char (info);
                info->state = ONE_TWO_SYM_LEX;
                break;
            case '\\':
                get_char (info);
                info->state = BACKSLASH;
                break;
            case '\"':
                get_char (info);
                info->state = IN_QUOTES;
                break;
            default:
                info->state = OTHER;
                break;
            }
            break;

        case ONE_SYM_LEX:
            lex *lex;
            switch (info->c) {
            case '<':
                lex = make_lex (LEX_INPUT);
                break;
            case ';':
                lex = make_lex (LEX_SEMICOLON);
                break;
            case '(':
                lex = make_lex (LEX_BRACKET_OPEN);
                break;
            case ')':
                lex = make_lex (LEX_BRACKET_CLOSE);
                break;
            default:
                fprintf (stderr, "Lexer: error in ONE_SYM_LEX.");
                exit (1);
            }
            /* We don't need buffer */
            get_char (info);
            return lex;

        case ONE_TWO_SYM_LEX:
            lex *lex;
            char old_char = info->c;
            /* We don't need buffer */
            get_char (info);
            switch (info->c) {
            case '>':
                lex = (old_char == info->c) ?
                    make_lex (LEX_APPEND) :
                    make_lex (LEX_OUTPUT);
                break;
            case '|':
                lex = (old_char == info->c) ?
                    make_lex (LEX_OR) :
                    make_lex (LEX_PIPE);
                break;
            case '(':
                lex = (old_char == info->c) ?
                    make_lex (LEX_AND) :
                    make_lex (LEX_BACKGROUND);
                break;
            default:
                fprintf (stderr, "Lexer: error in ONE_TWO_SYM_LEX.");
                exit (1);
            }
            info->state = START;
            return lex;

        case BACKSLASH:
            switch (info->c) {
            case 'a':
                add (info, '\a');
                break;
            case 'b':
                add (info, '\b');
                break;
            case 'f':
                add (info, '\f');
                break;
            case 'n':
                add (info, '\n');
                break;
            case 'r':
                add (info, '\r');
                break;
            case 't':
                add (info, '\t');
                break;
            case 'v':
                add (info, '\v');
                break;
            case ' ':
            case '\"':
            case '\\':
                add_current (info);
            case '\n':
                break;
            default:
                add (info, '\\');
                add_current (info);
            }
            get_char(info);
            info->state = START;
            break;

        case BACKSLASH_IN_QUOTES:
            /* TODO: what different from BACKSLASH? */
            /* Probably, absolutelly identical BACKSLASH,
             * but: info->state = IN_QUOTES */
            break;

        case IN_QUOTES:
            switch (info->c) {
            case '\\':
                get_char (info);
                info->state = BACKSLASH_IN_QUOTES;
                break;
            case '\"':
                get_char (info);
                info->state = START;
                break;
            default:
                add_current (info);
                get_char (info);
                break;
            }
            break;

        case OTHER:
            add_current (info);
            get_char (info);
            info->state = START;
            break;

        case ERROR:
            /* TODO */
            break;

        default:
            fprintf (stderr, "Lexer: error in main switch.");
            exit (1);
            break;
        }
    } while (true);
}


/* Returns readed (by getchar) symbols count */
/* TODO: remove
int read_block(char *str)
{
    int i;
    for (i = 0; i < BLOCK_SIZE; ++i) {
        *str = getchar();
        ++str;
        if (*str == TERM_SYM)
            break;
    }
    *str = '\0';
    return i + 1;
}
*/

/* return NULL if error */
/* TODO: remove
{
    strlist_ext *buffer = new_strlist_ext(REAL_BLOCK_SIZE);

    do {
        buffer->count_sym += read_block(e->last_item->str);
        add_item_to_strlist_ext(buffer);
        // возможен лишний блок в конце.
    } while(!eow);
    strlist_to_str
    match token type
    return token;
}
*/
