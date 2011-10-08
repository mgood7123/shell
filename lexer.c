#include "lexer.h"

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

/* #define DEBUG_LEXER */

void print_state (const char *state_name, int c)
{
    fprintf (stderr, "Lexer: %s; ", state_name);

    switch (c) {
    case '\n':
        fprintf(stderr, "\'\\n\';\n");
        break;
    case EOF:
        fprintf(stderr, "\'EOF\';\n");
        break;
    default:
        fprintf(stderr, "\'%c\';\n", c);
        break;
    }
}

void print_lex (lexeme *lex)
{
    printf ("!!! Lexeme: ");
    switch (lex->type) {
    case LEX_INPUT:         /* '<'  */
        printf ("[<]\n");
        break;
    case LEX_OUTPUT:        /* '>'  */
        printf ("[>]\n");
        break;
    case LEX_APPEND:        /* '>>' */
        printf ("[>>]\n");
        break;
    case LEX_PIPE:          /* '|'  */
        printf ("[|]\n");
        break;
    case LEX_OR:            /* '||' */
        printf ("[||]\n");
        break;
    case LEX_BACKGROUND:    /* '&'  */
        printf ("[&]\n");
        break;
    case LEX_AND:           /* '&&' */
        printf ("[&&]\n");
        break;
    case LEX_SEMICOLON:     /* ';'  */
        printf ("[;]\n");
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
        printf ("[(]\n");
        break;
    case LEX_BRACKET_CLOSE: /* ')'  */
        printf ("[)]\n");
        break;
    case LEX_REVERSE:       /* '`'  */
        printf ("[`]\n");
        break;
    case LEX_WORD:     /* all different */
        printf ("[WORD:%s]\n", lex->str);
        break;
    case LEX_EOLINE:        /* '\n' */
        printf ("[EOLINE]\n");
        break;
    case LEX_EOFILE:         /* EOF  */
        printf ("[EOFILE]\n");
        break;
    }
}

void deferred_get_char (lexer_info *linfo)
{
    linfo->get_next_char = 1;
}

void get_char (lexer_info *linfo)
{
    linfo->c = getchar();
}

void init_lexer (lexer_info *linfo)
{
/*  lexer_info *linfo =
        (lexer_info *) malloc (sizeof (lexer_info)); */

    deferred_get_char (linfo);
    linfo->state = ST_START;
}

lexeme *make_lex (type_of_lex type)
{
    lexeme *lex = (lexeme *) malloc (sizeof (lexeme));
/*    lex->next = NULL; */
    lex->type = type;
    lex->str = NULL;
    return lex;
}

void destroy_lex (lexeme *lex)
{
    if (lex->str != NULL)
        free (lex->str);
    free (lex);
}

lexeme *get_lex (lexer_info *linfo)
{
    lexeme *lex = NULL;
    buffer buf;

    new_buffer (&buf);

    do {
        if (linfo->get_next_char) {
            linfo->get_next_char = 0;
            get_char (linfo);
        }

        switch (linfo->state) {
        case ST_START:
#ifdef DEBUG_LEXER
            print_state ("ST_START", linfo->c);
#endif

            switch (linfo->c) {
            case EOF:
            case '\n':
                linfo->state = ST_EOLN_EOF;
                break;
            case ' ':
            case '\t':
                deferred_get_char (linfo);
                break;
            case '<':
            case ';':
            case '(':
            case ')':
            case '`':
                linfo->state = ST_ONE_SYM_LEX;
                break;
            case '>':
            case '|':
            case '&':
                linfo->state = ST_ONE_TWO_SYM_LEX;
                break;
            case '\\':
            case '\"':
            default:
                linfo->state = ST_WORD;
                break;
            }
            break;

        case ST_ONE_SYM_LEX:
#ifdef DEBUG_LEXER
            print_state ("ST_ONE_SYM_LEX", linfo->c);
#endif

            switch (linfo->c) {
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
            case '`':
                lex = make_lex (LEX_REVERSE);
                break;
            default:
                fprintf (stderr, "Lexer: error in ST_ONE_SYM_LEX;");
                print_state ("ST_ONE_SYM_LEX", linfo->c);
                exit (1);
            }
            /* We don't need buffer */
            deferred_get_char (linfo);
            linfo->state = ST_START;
            return lex;

        case ST_ONE_TWO_SYM_LEX:
#ifdef DEBUG_LEXER
            print_state ("ST_ONE_TWO_SYM_LEX", linfo->c);
#endif

            if (buf.count_sym == 0) {
                add_to_buffer (&buf, linfo->c);
                deferred_get_char (linfo);
            } else if (buf.count_sym == 1) {
                /* TODO: make it more pretty */
                char prev_c = get_last_from_buffer (&buf);
                clear_buffer (&buf);
                switch (prev_c) {
                case '>':
                    lex = (prev_c == linfo->c) ?
                        make_lex (LEX_APPEND) :
                        make_lex (LEX_OUTPUT);
                    break;
                case '|':
                    lex = (prev_c == linfo->c) ?
                        make_lex (LEX_OR) :
                        make_lex (LEX_PIPE);
                    break;
                case '&':
                    lex = (prev_c == linfo->c) ?
                        make_lex (LEX_AND) :
                        make_lex (LEX_BACKGROUND);
                    break;
                default:
                    fprintf (stderr, "Lexer: error (type 1) in ST_ONE_TWO_SYM_LEX;");
                    print_state ("ST_ONE_TWO_SYM_LEX", linfo->c);
                    exit (1);
                }
                if (prev_c == linfo->c)
                    deferred_get_char (linfo);
                linfo->state = ST_START;
                return lex;
            } else {
                fprintf (stderr, "Lexer: error (type 2) in ST_ONE_TWO_SYM_LEX;");
                print_state ("ST_ONE_TWO_SYM_LEX", linfo->c);
                exit (1);
            }
            break;


        case ST_BACKSLASH:
#ifdef DEBUG_LEXER
            print_state ("ST_BACKSLASH", linfo->c);
#endif

            switch (linfo->c) {
            case EOF:
                linfo->state = ST_ERROR;
                continue;
            case '\n':
                /* Ignore newline symbol */
                break;
/* Non bash-like behaviour. In bash substitution
 * makes in $'string' costruction. */
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
            default:
                add_to_buffer (&buf, linfo->c);
                break;
            }
            deferred_get_char (linfo);
            linfo->state = ST_WORD;
            break;

        case ST_BACKSLASH_IN_QUOTES:
#ifdef DEBUG_LEXER
            print_state ("ST_BACKSLASH_IN_QUOTES", linfo->c);
#endif

            switch (linfo->c) {
            case EOF:
                linfo->state = ST_ERROR;
                break;
            case '\"':
                add_to_buffer (&buf, linfo->c);
                break;
            default:
                add_to_buffer (&buf, '\\');
                add_to_buffer (&buf, linfo->c);
                break;
            }
            deferred_get_char (linfo);
            linfo->state = ST_IN_QUOTES;
            break;

        case ST_IN_QUOTES:
#ifdef DEBUG_LEXER
            print_state ("ST_IN_QUOTES", linfo->c);
#endif

            switch (linfo->c) {
            case EOF:
                linfo->state = ST_ERROR;
                continue;
            case '\\':
                deferred_get_char (linfo);
                linfo->state = ST_BACKSLASH_IN_QUOTES;
                break;
            case '\"':
                deferred_get_char (linfo);
                linfo->state = ST_WORD;
                break;
            default:
                add_to_buffer (&buf, linfo->c);
                deferred_get_char (linfo);
                break;
            }
            break;

        case ST_WORD:
#ifdef DEBUG_LEXER
            print_state ("ST_WORD", linfo->c);
#endif

            switch (linfo->c) {
            case EOF:
            case '\n':
            case ' ':
            case '\t':
            case '<':
            case ';':
            case '(':
            case ')':
            case '>':
            case '|':
            case '&':
            case '`':
                linfo->state = ST_START;
                lex = make_lex (LEX_WORD);
                lex->str = convert_to_string (&buf, 1);
                clear_buffer (&buf);
                return lex;
            case '\\':
                deferred_get_char (linfo);
                linfo->state = ST_BACKSLASH;
                break;
            case '\"':
                deferred_get_char (linfo);
                linfo->state = ST_IN_QUOTES;
                break;
            default:
                add_to_buffer (&buf, linfo->c);
                deferred_get_char (linfo);
            }
            break;

        case ST_ERROR:
            print_state ("ST_ERROR", linfo->c);
            clear_buffer (&buf);
            linfo->get_next_char = 0;
            linfo->state = ST_START;
            /* TODO: read to '\n' or EOF */
            return NULL;

        case ST_EOLN_EOF:
#ifdef DEBUG_LEXER
            print_state ("ST_EOLN_EOF", linfo->c);
#endif

            switch (linfo->c) {
            case '\n':
                lex = make_lex (LEX_EOLINE);
                break;
            case EOF:
                lex = make_lex (LEX_EOFILE);
                break;
            default:
                fprintf (stderr, "Lexer: error in ST_EOLN_EOF;");
                print_state ("ST_EOLN_EOF", linfo->c);
                exit (1);
            }
            deferred_get_char (linfo);
            linfo->state = ST_START;
            return lex;

        default:
            print_state ("unrecognized state", linfo->c);
            exit (1);
            break;
        }
    } while (1);
}

/*
gcc -g -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g -Wall -ansi -pedantic lexer.c buffer.o -o lexer
*/

/*
int main ()
{
    lexer_info linfo;
    init_lexer (&linfo);

    do {
        lexeme *lex = get_lex (&linfo);
        print_lex (lex);
        if (lex->type == LEX_EOFILE)
            return 0;
        destroy_lex (lex);

        if (linfo.state == ST_ERROR) {
            fprintf(stderr, "(>_<)\n");
            continue;
        }
    } while (1);
}
*/
