#include <stdlib.h>

#include "lexer.h"
#include "buffer.h"
#include "utils.h"

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

void print_lex (FILE *stream, lexeme *lex)
{
    fprintf (stream, "!!! Lexeme: ");
    switch (lex->type) {
    case LEX_INPUT:         /* '<'  */
        fprintf (stream, "[<]\n");
        break;
    case LEX_OUTPUT:        /* '>'  */
        fprintf (stream, "[>]\n");
        break;
    case LEX_APPEND:        /* '>>' */
        fprintf (stream, "[>>]\n");
        break;
    case LEX_PIPE:          /* '|'  */
        fprintf (stream, "[|]\n");
        break;
    case LEX_OR:            /* '||' */
        fprintf (stream, "[||]\n");
        break;
    case LEX_BACKGROUND:    /* '&'  */
        fprintf (stream, "[&]\n");
        break;
    case LEX_AND:           /* '&&' */
        fprintf (stream, "[&&]\n");
        break;
    case LEX_SEMICOLON:     /* ';'  */
        fprintf (stream, "[;]\n");
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
        fprintf (stream, "[(]\n");
        break;
    case LEX_BRACKET_CLOSE: /* ')'  */
        fprintf (stream, "[)]\n");
        break;
    case LEX_REVERSE:       /* '`'  */
        fprintf (stream, "[`]\n");
        break;
    case LEX_WORD:     /* all different */
        fprintf (stream, "[WORD:%s]\n", lex->str);
        break;
    case LEX_EOLINE:        /* '\n' */
        fprintf (stream, "[EOLINE]\n");
        break;
    case LEX_EOFILE:        /* EOF  */
        fprintf (stream, "[EOFILE]\n");
        break;
    case LEX_ERROR:    /* error in lexer */
        fprintf (stream, "[ERROR]\n");
        break;
    }
}

void deferred_get_char (lexer_info *linfo)
{
    linfo->get_next_char = 1;
}

void get_char (lexer_info *linfo)
{
    linfo->c = getchar ();
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

lexeme *st_start (lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
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

    return NULL;
}

lexeme *st_one_sym_lex (lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
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
        exit (ES_LEXER_INCURABLE_ERROR);
    }

    /* We don't need buffer */
    deferred_get_char (linfo);
    linfo->state = ST_START;
    return lex;
}

lexeme *st_one_two_sym_lex (lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
    print_state ("ST_ONE_TWO_SYM_LEX", linfo->c);
#endif

    if (buf->count_sym == 0) {
        add_to_buffer (buf, linfo->c);
        deferred_get_char (linfo);
    } else if (buf->count_sym == 1) {
        /* TODO: make it more pretty */
        char prev_c = get_last_from_buffer (buf);
        clear_buffer (buf);
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
            exit (ES_LEXER_INCURABLE_ERROR);
        }
        if (prev_c == linfo->c)
            deferred_get_char (linfo);
        linfo->state = ST_START;
        return lex;
    } else {
        fprintf (stderr, "Lexer: error (type 2) in ST_ONE_TWO_SYM_LEX;");
        print_state ("ST_ONE_TWO_SYM_LEX", linfo->c);
        exit (ES_LEXER_INCURABLE_ERROR);
    }

    return lex;
}

lexeme *st_backslash (lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state ("ST_BACKSLASH", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        return NULL;
    case '\n':
        /* Ignore newline symbol */
        print_prompt2 ();
        break;
/* Non bash-like behaviour. In bash substitution
 * makes in $'string' costruction. */
    case 'a':
        add_to_buffer (buf, '\a');
        break;
    case 'b':
        add_to_buffer (buf, '\b');
        break;
    case 'f':
        add_to_buffer (buf, '\f');
        break;
    case 'n':
        add_to_buffer (buf, '\n');
        break;
    case 'r':
        add_to_buffer (buf, '\r');
        break;
    case 't':
        add_to_buffer (buf, '\t');
        break;
    case 'v':
        add_to_buffer (buf, '\v');
        break;
    default:
        add_to_buffer (buf, linfo->c);
        break;
    }

    deferred_get_char (linfo);
    linfo->state = ST_WORD;

    return NULL;
}

lexeme *st_backslash_in_quotes (lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state ("ST_BACKSLASH_IN_QUOTES", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        break;
    case '\"':
        add_to_buffer (buf, linfo->c);
        break;
    case '\n':
        print_prompt2 ();
        /* fallthrough */
    default:
        add_to_buffer (buf, '\\');
        add_to_buffer (buf, linfo->c);
        break;
    }
    deferred_get_char (linfo);
    linfo->state = ST_IN_QUOTES;

    return NULL;
}

lexeme *st_in_quotes (lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state ("ST_IN_QUOTES", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        return NULL;
    case '\\':
        deferred_get_char (linfo);
        linfo->state = ST_BACKSLASH_IN_QUOTES;
        break;
    case '\"':
        deferred_get_char (linfo);
        linfo->state = ST_WORD;
        break;
    case '\n':
        print_prompt2 ();
        /* fallthrough */
    default:
        add_to_buffer (buf, linfo->c);
        deferred_get_char (linfo);
        break;
    }

    return NULL;
}

lexeme *st_word (lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
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
        lex->str = convert_to_string (buf, 1);
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
        add_to_buffer (buf, linfo->c);
        deferred_get_char (linfo);
    }

    return lex;
}

lexeme *st_error (lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

    lex = make_lex (LEX_ERROR);
    print_state ("ST_ERROR", linfo->c);
    clear_buffer (buf);
    linfo->get_next_char = 0;
    linfo->state = ST_START;
    /* TODO: read to '\n' or EOF (flush read buffer) */

    return lex;
}

lexeme *st_eoln_eof (lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
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
        exit (ES_LEXER_INCURABLE_ERROR);
    }

    deferred_get_char (linfo);
    linfo->state = ST_START;

    return lex;
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
            lex = st_start (linfo, &buf);
            break;

        case ST_ONE_SYM_LEX:
            lex = st_one_sym_lex (linfo, &buf);
            break;

        case ST_ONE_TWO_SYM_LEX:
            lex = st_one_two_sym_lex (linfo, &buf);
            break;

        case ST_BACKSLASH:
            lex = st_backslash (linfo, &buf);
            break;

        case ST_BACKSLASH_IN_QUOTES:
            lex = st_backslash_in_quotes (linfo, &buf);
            break;

        case ST_IN_QUOTES:
            lex = st_in_quotes (linfo, &buf);
            break;

        case ST_WORD:
            lex = st_word (linfo, &buf);
            break;

        case ST_ERROR:
            lex = st_error (linfo, &buf);
            break;

        case ST_EOLN_EOF:
            lex = st_eoln_eof (linfo, &buf);
            break;
        } /* switch */
    } while (lex == NULL);

    return lex;
}

/*
gcc -g -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g -Wall -ansi -pedantic -c utils.c -o utils.o &&
gcc -g -Wall -ansi -pedantic lexer.c buffer.o utils.o -o lexer
*/

#if 0
int main ()
{
    lexer_info linfo;
    init_lexer (&linfo);

    do {
        lexeme *lex = get_lex (&linfo);
        print_lex (stdout, lex);
        if (lex->type == LEX_EOFILE)
            return 0;
        destroy_lex (lex);

        if (linfo.state == ST_ERROR) {
            fprintf(stderr, "(>_<)\n");
            continue;
        }
    } while (1);
}
#endif
