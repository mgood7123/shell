#include "lexer.h"

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

void get_char (lexer_info *info)
{
    info->c = getchar();
}

void init_lexer (lexer_info *info)
{
/*    lexer_info *info =
        (lexer_info *) malloc (sizeof (lexer_info)); */
    info->state = START;
    get_char(info);
}

lexeme *make_lex (type_of_lex type)
{
    lexeme *lex = (lexeme *) malloc (sizeof (lexeme));
    lex->next = NULL;
    lex->type = type;
    lex->str = NULL;
    return lex;
}

lexeme *get_lex(lexer_info *info)
{
    lexeme *lex = NULL;
    buffer buf;
    char old_char; /* Used only in ONE_TWO_SYM_LEX */

    new_buffer (&buf);

    do {
        switch (info->state) {
        case START:
            switch (info->c) {
            case EOF:
            case '\n':
                /* TODO return buffer */
                info->state = END_OF;
                return lex;
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
            info->state = START;
            return lex;

        case ONE_TWO_SYM_LEX:
            old_char = info->c;
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
            case '&':
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
                add_to_buffer (&buf, '\a');
                break;
            case 'b':
                add_to_buffer (&buf, '\b');
                break;
            case 'f':
                add_to_buffer (&buf, '\f');
                break;
            case 'n':
                add_to_buffer (&buf, '\n');
                break;
            case 'r':
                add_to_buffer (&buf, '\r');
                break;
            case 't':
                add_to_buffer (&buf, '\t');
                break;
            case 'v':
                add_to_buffer (&buf, '\v');
                break;
            case ' ':
            case '\"':
            case '\\':
                add_to_buffer (&buf, info->c);
            case '\n':
                /* Ignore newline symbol */
                break;
            default:
                /* Substitution not found */
                add_to_buffer (&buf, '\\');
                add_to_buffer (&buf, info->c);
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
                add_to_buffer (&buf, info->c);
                get_char (info);
                break;
            }
            break;

        case OTHER:
            add_to_buffer (&buf, info->c);
            get_char (info);
            info->state = START;
            break;

        case ERROR:
            /* TODO */
            break;

        case END_OF:
            switch (info->c) {
            case '\n':
                lex = make_lex(LEX_EOLINE);
                break;
            case EOF:
                lex = make_lex(LEX_EOFILE);
                break;
            }
            info->state = START;
            return lex;

        default:
            fprintf (stderr, "Lexer: error in main switch.");
            exit (1);
            break;
        }
    } while (1);
}
