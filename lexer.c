#include <stdlib.h>

#include "lexer.h"
#include "buffer.h"
#include "utils.h"

/* char * string = "<\n<<\n<<<"; 
char * string = "cat <<EOF -n\nHELLO\nEOF";
*/
char * string = " ls -l \"a \\\" b c\"\n"
"cat <<EOF -n ; cat <<K -q ; we are done\n"
"HELLO\n"
"cat <<EOF -n\nHELLO\n"
" ls -l \"a \\\" b c\"\n"
"it works!\n"
"EOF\n"
"help\n"
"K"
;

int string_pos = 0;

/* heredoc nesting */

void heredoc_undefined(void) {
  heredoc = calloc(1, sizeof(struct heredoc));
  heredoc->count = 1;
  heredoc[heredoc->count-1].pending = 0;
  heredoc[heredoc->count-1].accepting = 0;
  heredoc[heredoc->count-1].final = 0;
  heredoc[heredoc->count-1].id = NULL;
  heredoc[heredoc->count-1].last_char = 0;
}

void heredoc_add(void) {
	
	if (!heredoc) {
		heredoc_undefined();
	}
	
	if (heredoc->count != 1 || heredoc[heredoc->count-1].id) {
		struct heredoc * heredoctmp = realloc(heredoc, sizeof(struct heredoc)*(heredoc->count+1));
		if (heredoctmp != NULL) heredoc = heredoctmp;
		else abort();
		heredoc->count++;
	}
	heredoc[heredoc->count-1].pending = 0;
	heredoc[heredoc->count-1].accepting = 0;
	heredoc[heredoc->count-1].final = 0;
	heredoc[heredoc->count-1].id = NULL;
}

void heredoc_undo(void) {
	if (!heredoc) {
		heredoc_undefined();
	}
	struct heredoc * heredoctmp = realloc(heredoc, sizeof(struct heredoc)*(heredoc->count-1));
	if (heredoctmp != NULL) heredoc = heredoctmp;
	else abort();
	memset(&heredoc[heredoc->count], 0, sizeof(struct heredoc));
	heredoc->count--;
	if (!heredoc->count) {
		free(heredoc);
		heredoc = NULL;
	}
}

void heredoc_remove(void) {
	if (!heredoc) return;
	int count = 0;
	int tmp_count = heredoc->count;
	int ii;
	for (ii = 0; ii < tmp_count; ii++) {
		if (ii+2 <= tmp_count) {
			heredoc[ii] = heredoc[ii+1];
		} else {
			memset(&heredoc[ii], 0, sizeof(struct heredoc));
			count++;
		}
	}
	heredoc->count = tmp_count;
	
	/*
	 
	 avoid the case of:
	 
	 $ man realloc
	 ...
	 void *realloc(void *ptr, size_t size);
	 ...
	 if size is equal to zero, and ptr is not NULL, then the call is equivalent to  free(ptr).
	 ...
	
	*/
	
	struct heredoc * heredoctmp = heredoc->count-count != 0 ? realloc(heredoc, sizeof(struct heredoc)*(heredoc->count-count)) : NULL;
	heredoc->count-=count;
	if (heredoctmp != NULL) heredoc = heredoctmp;
	else if (heredoc->count != 0){
		puts("resulting in a NULL heredoc with a non zero heredoc count, aborting");
		abort();
	}
	if (!heredoc->count) {
		free(heredoc);
		heredoc = NULL;
	}
}

void heredoc_free(void) {
	if (!heredoc) {
		return;
	}
	int i;
	for (i = 0; i < heredoc->count; i++) {
		memset(&heredoc[i], 0, sizeof(struct heredoc));
	}
	heredoc->count = 0;
	free(heredoc);
	heredoc = NULL;
}


void print_state(const char *state_name, int c)
{
    fprintf(stderr, "Lexer: %s; ", state_name);

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


char * lexer_tokens_current_token() {
	int s = env__size(lexer_tokens.token);
	if (s > 0) return lexer_tokens.token[s-1];
	return NULL;
}

char * lexer_tokens_current_token_type() {
	int s = env__size(lexer_tokens.token_type);
	if (s > 0) return lexer_tokens.token_type[s-1];
	return NULL;
}

char * lexer_tokens_current_token_column() {
	int s = env__size(lexer_tokens.token_column);
	if (s > 0) return lexer_tokens.token_column[s-1];
	return NULL;
}

char * lexer_tokens_current_token_line() {
	int s = env__size(lexer_tokens.token_line);
	if (s > 0) return lexer_tokens.token_line[s-1];
	return NULL;
}

char * lexer_tokens_last_token() {
	int s = env__size(lexer_tokens.token);
	if (s > 1) return lexer_tokens.token[s-2];
	return NULL;
}

char * lexer_tokens_last_token_type() {
	int s = env__size(lexer_tokens.token_type);
	if (s > 1) return lexer_tokens.token_type[s-2];
	return NULL;
}

char * lexer_tokens_last_token_column() {
	int s = env__size(lexer_tokens.token_column);
	if (s > 1) return lexer_tokens.token_column[s-2];
	return NULL;
}

char * lexer_tokens_last_token_line() {
	int s = env__size(lexer_tokens.token_line);
	if (s > 1) return lexer_tokens.token_line[s-2];
	return NULL;
}

char * lexer_tokens_last_token_before_that() {
	int s = env__size(lexer_tokens.token);
	if (s > 2) return lexer_tokens.token[s-3];
	return NULL;
}

char * lexer_tokens_last_token_type_before_that() {
	int s = env__size(lexer_tokens.token_type);
	if (s > 2) return lexer_tokens.token_type[s-3];
	return NULL;
}

char * lexer_tokens_last_token_column_before_that() {
	int s = env__size(lexer_tokens.token_column);
	if (s > 2) return lexer_tokens.token_column[s-3];
	return NULL;
}

char * lexer_tokens_last_token_line_before_that() {
	int s = env__size(lexer_tokens.token_line);
	if (s > 2) return lexer_tokens.token_line[s-3];
	return NULL;
}

int which_token_int(char * desired) {
	if (token) if (strcmp(token, desired) == 0) return 0;
	if (last_token) if (strcmp(last_token, desired) == 0) return 0;
	if (last_token_before_that) if (strcmp(last_token_before_that, desired) == 0) return 0;
	return -1;
}

int which_token_type_int(char * desired) {
	if (token_type) if (strcmp(token_type, desired) == 0) return 0;
	if (last_token_type) if (strcmp(last_token_type, desired) == 0) return 0;
	if (last_token_type_before_that) if (strcmp(last_token_type_before_that, desired) == 0) return 0;
	return -1;
}

int which_token_order_int(char * desired1, char * desired2) {
	if (token) if (strcmp(token, desired2) == 0) {
		if (last_token) if (strcmp(last_token, desired1) == 0) {
			return 0;
		}
		return -1;
	}
	if (last_token) if (strcmp(last_token, desired2) == 0) {
		if (last_token_before_that) if (strcmp(last_token_before_that, desired1) == 0) {
			return 0;
		}
		return -1;
	}
	return -1;
}

int which_token_type_order_int(char * desired1, char * desired2) {
	if (token_type) if (strcmp(token_type, desired2) == 0) {
		if (last_token_type) if (strcmp(last_token_type, desired1) == 0) {
			return 0;
		}
		return -1;
	}
	if (last_token_type) if (strcmp(last_token_type, desired2) == 0) {
		if (last_token_type_before_that) if (strcmp(last_token_type_before_that, desired1) == 0) {
			return 0;
		}
		return -1;
	}
	return -1;
}

int which_token_position_order_int(int desired1, int desired2) {
	if (token_column == desired2) {
		if(last_token_column == desired1) {
			return 0;
		}
		return -1;
	}
	else if (last_token_column == desired2) {
		if (last_token_before_that_column == desired1) {
			return 0;
		}
		return -1;
	}
	else return -1;
}

int which_token_line_order_int(int desired1, int desired2) {
	if (token_line == desired2) {
		if (last_token_line == desired1) {
			return 0;
		}
		return -1;
	}
	else if (last_token_line == desired2) {
		if (last_token_before_that_line == desired1) {
			return 0;
		}
		return -1;
	}
	else return -1;
}

char * which_token_char(char * desired) {
	if (which_token_int(desired) == -1) return "(null)";
	if (token) if (strcmp(token, desired) == 0) return token;
	if (last_token) if (strcmp(last_token, desired) == 0) return last_token;
	if (last_token_before_that) if (strcmp(last_token_before_that, desired) == 0) return last_token_before_that;
	return NULL;
}

char * which_token_type_char(char * desired) {
	if (which_token_type_int(desired) == -1) return "(null)";
	if (token_type) if (strcmp(token_type, desired) == 0) return token_type;
	if (last_token_type) if (strcmp(last_token_type, desired) == 0) return last_token_type;
	if (last_token_type_before_that) if (strcmp(last_token_type_before_that, desired) == 0) return last_token_type_before_that;
	return NULL;
}

void token_info(void) {
    ps(token);
    ps(token_type);
    pi(token_column);
    pi(token_line);
	ps(last_token);
	ps(last_token_type);
	pi(last_token_column);
	pi(last_token_line);
	ps(last_token_before_that);
	ps(last_token_type_before_that);
	pi(last_token_before_that_column);
	pi(last_token_before_that_line);
}

void store_lex(lexeme *lex)
{
	str_new(column);
	str_insert_int(column, lexer_tokens.column_s);
	lexer_tokens.token_column = env__add2(lexer_tokens.token_column, column.string);
	str_free(column);

	str_new(line);
	str_insert_int(line, lexer_tokens.line_s);
	lexer_tokens.token_line = env__add2(lexer_tokens.token_line, line.string);
	str_free(line);
	
    switch (lex->type) {
    case LEX_INPUT:         /* '<'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, "<");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_INPUT");
        break;
    case LEX_HEREDOC:       /* '<<' */
		lexer_tokens.token = env__add2(lexer_tokens.token, "<<");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_HEREDOC");
        break;
    case LEX_HEREDOC_WORD:       /* '<<' */
		lexer_tokens.token = env__add2(lexer_tokens.token, lex->str);
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_HEREDOC_WORD");
        break;
    case LEX_HERESTRING:    /* '<<<'*/
		lexer_tokens.token = env__add2(lexer_tokens.token, "<<<");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_HERESTRING");
        break;
    case LEX_OUTPUT:        /* '>'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, ">");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_OUTPUT");
        break;
    case LEX_APPEND:        /* '>>' */
		lexer_tokens.token = env__add2(lexer_tokens.token, ">>");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_APPEND");
        break;
    case LEX_PIPE:          /* '|'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, "|");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_PIPE");
        break;
    case LEX_OR:            /* '||' */
		lexer_tokens.token = env__add2(lexer_tokens.token, "||");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_OR");
        break;
    case LEX_BACKGROUND:    /* '&'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, "&");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_BACKGROUND");
        break;
    case LEX_AND:           /* '&&' */
		lexer_tokens.token = env__add2(lexer_tokens.token, "&&");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_AND");
        break;
    case LEX_SEMICOLON:     /* ';'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, ";");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_SEMICOLON");
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
		lexer_tokens.token = env__add2(lexer_tokens.token, "(");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_BRACKET_OPEN");
        break;
    case LEX_BRACKET_CLOSE: /* ')'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, ")");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_BRACKET_CLOSE");
        break;
    case LEX_REVERSE:       /* '`'  */
		lexer_tokens.token = env__add2(lexer_tokens.token, "`");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_REVERSE");
        break;
    case LEX_WORD:     /* all different */
		lexer_tokens.token = env__add2(lexer_tokens.token, lex->str);
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_WORD");
        break;
    case LEX_EOLINE:        /* '\n' */
		if (heredoc) if (heredoc[0].pending) {
			heredoc[0].pending = 0;
			heredoc[0].accepting = 1;
		}
		lexer_tokens.token = env__add2(lexer_tokens.token, "EOLINE");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_EOLINE");
        break;
    case LEX_EOFILE:        /* EOF  */
		lexer_tokens.token = env__add2(lexer_tokens.token, "EOFILE");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_EOFILE");
        break;
    case LEX_ERROR:    /* error in lexer */
		lexer_tokens.token = env__add2(lexer_tokens.token, "ERROR");
		lexer_tokens.token_type = env__add2(lexer_tokens.token_type, "LEX_ERROR");
        break;
    }
    #define atoin(x) x != NULL ? atoi(x) : -1
    token 							= lexer_tokens_current_token();
    token_type 						= lexer_tokens_current_token_type();
    token_column 					= atoin(lexer_tokens_current_token_column());
    token_line 						= atoin(lexer_tokens_current_token_line());
	last_token 						= lexer_tokens_last_token();
	last_token_type 				= lexer_tokens_last_token_type();
	last_token_column 				= atoin(lexer_tokens_last_token_column());
	last_token_line 				= atoin(lexer_tokens_last_token_line());
	last_token_before_that 			= lexer_tokens_last_token_before_that();
	last_token_type_before_that 	= lexer_tokens_last_token_type_before_that();
	last_token_before_that_column 	= atoin(lexer_tokens_last_token_column_before_that());
	last_token_before_that_line 	= atoin(lexer_tokens_last_token_line_before_that());

    if (env__size(lexer_tokens.token_type) == env__size(lexer_tokens.token)) {
#ifdef LEXER_DEBUG
		token_info();
#endif
		if (which_token_type_order_int("LEX_HEREDOC", "LEX_WORD") == 0) {
			heredoc_add();
			heredoc[heredoc->count-1].pending=1;
			heredoc[heredoc->count-1].id = token;
		}
	} else puts("lexer_tokens mismatch");
}

void print_lex(FILE *stream, lexeme *lex)
{
    fprintf(stream, "!!! Lexeme: ");
    switch (lex->type) {
    case LEX_INPUT:         /* '<'  */
        fprintf(stream, "[<]\n");
        break;
    case LEX_HEREDOC:       /* '<<' */
        fprintf(stream, "[HEREDOC IDENTIFIER:<<]\n");
        break;
    case LEX_HEREDOC_WORD:  /* '<<' */
        fprintf(stream, "[HEREDOC_WORD:%s]\n", lex->str);
        break;
    case LEX_HERESTRING:    /* '<<<'*/
        fprintf(stream, "[<<<]\n");
        break;
    case LEX_OUTPUT:        /* '>'  */
        fprintf(stream, "[>]\n");
        break;
    case LEX_APPEND:        /* '>>' */
        fprintf(stream, "[>>]\n");
        break;
    case LEX_PIPE:          /* '|'  */
        fprintf(stream, "[|]\n");
        break;
    case LEX_OR:            /* '||' */
        fprintf(stream, "[||]\n");
        break;
    case LEX_BACKGROUND:    /* '&'  */
        fprintf(stream, "[&]\n");
        break;
    case LEX_AND:           /* '&&' */
        fprintf(stream, "[&&]\n");
        break;
    case LEX_SEMICOLON:     /* ';'  */
        fprintf(stream, "[;]\n");
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
        fprintf(stream, "[(]\n");
        break;
    case LEX_BRACKET_CLOSE: /* ')'  */
        fprintf(stream, "[)]\n");
        break;
    case LEX_REVERSE:       /* '`'  */
        fprintf(stream, "[`]\n");
        break;
    case LEX_WORD:     /* all different */
        fprintf(stream, "[WORD:%s]\n", lex->str);
        break;
    case LEX_EOLINE:        /* '\n' */
        fprintf(stream, "[EOLINE]\n");
        break;
    case LEX_EOFILE:        /* EOF  */
        fprintf(stream, "[EOFILE]\n");
        break;
    case LEX_ERROR:    /* error in lexer */
        fprintf(stream, "[ERROR]\n");
        break;
    }
}

void deferred_get_char(lexer_info *linfo)
{
    linfo->get_next_char = 1;
}

void get_char(lexer_info *linfo)
{
	if (string_pos == 0) {
		printf("lexing string: %s\n", string);
		lexer_tokens.line = 0;
		lexer_tokens.column = 0;
	}
	if (string[string_pos] == 0) linfo->c = EOF;
	else linfo->c = string[string_pos];
	string_pos++;
	/* linfo->c = getchar(); */
	if (linfo->c == '\n') {
		lexer_tokens.line++;
		lexer_tokens.column = 0;
	}
	else lexer_tokens.column++;

}

void init_lexer(lexer_info *linfo)
{
/*  lexer_info *linfo =
        (lexer_info *) malloc(sizeof(lexer_info)); */

    deferred_get_char(linfo);
    linfo->state = ST_START;
}

lexeme *make_lex(type_of_lex type)
{
    lexeme *lex = (lexeme *) malloc(sizeof(lexeme));
/*    lex->next = NULL; */
    lex->type = type;
    lex->str = NULL;
    return lex;
}

void destroy_lex(lexeme *lex)
{
    if (lex->str != NULL)
        free(lex->str);
    free(lex);
}

lexeme *st_start(lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state("ST_START", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
    case '\n':
        linfo->state = ST_EOLN_EOF;
        break;
    case ' ':
    case '\t':
        deferred_get_char(linfo);
        break;
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
    case '<':
        linfo->state = ST_ONE_TWO_THREE_SYM_LEX;
        break;
    case '\\':
    case '\"':
    default:
        linfo->state = ST_WORD;
        break;
    }

    return NULL;
}

lexeme *st_one_sym_lex(lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
    print_state("ST_ONE_SYM_LEX", linfo->c);
#endif

    switch (linfo->c) {
    case ';':
        lex = make_lex(LEX_SEMICOLON);
        break;
    case '(':
        lex = make_lex(LEX_BRACKET_OPEN);
        break;
    case ')':
        lex = make_lex(LEX_BRACKET_CLOSE);
        break;
    case '`':
        lex = make_lex(LEX_REVERSE);
        break;
    default:
        fprintf(stderr, "Lexer: error in ST_ONE_SYM_LEX;");
        print_state("ST_ONE_SYM_LEX", linfo->c);
        exit(ES_LEXER_INCURABLE_ERROR);
    }

    /* We don't need buffer */
    deferred_get_char(linfo);
    linfo->state = ST_START;
    return lex;
}

lexeme *st_one_two_sym_lex(lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
    print_state("ST_ONE_TWO_SYM_LEX", linfo->c);
#endif

    if (buf->count_sym == 0) {
        add_to_buffer(buf, linfo->c);
        deferred_get_char(linfo);
    } else if (buf->count_sym == 1) {
        /* TODO: make it more pretty */
        char prev_c = get_last_from_buffer(buf);
        clear_buffer(buf);
        switch (prev_c) {
        case '>':
            lex = (prev_c == linfo->c) ?
                make_lex(LEX_APPEND) :
                make_lex(LEX_OUTPUT);
            break;
        case '|':
            lex = (prev_c == linfo->c) ?
                make_lex(LEX_OR) :
                make_lex(LEX_PIPE);
            break;
        case '&':
            lex = (prev_c == linfo->c) ?
                make_lex(LEX_AND) :
                make_lex(LEX_BACKGROUND);
            break;
        default:
            fprintf(stderr, "Lexer: error (type 1) in ST_ONE_TWO_SYM_LEX;");
            print_state("ST_ONE_TWO_SYM_LEX", linfo->c);
            exit(ES_LEXER_INCURABLE_ERROR);
        }
        if (prev_c == linfo->c)
            deferred_get_char(linfo);
        linfo->state = ST_START;
        return lex;
    } else {
        fprintf(stderr, "Lexer: error (type 2) in ST_ONE_TWO_SYM_LEX;");
        print_state("ST_ONE_TWO_SYM_LEX", linfo->c);
        exit(ES_LEXER_INCURABLE_ERROR);
    }

    return lex;
}

char st_one_two_three_sym_lex_prev_c = 0;

lexeme *st_one_two_three_sym_lex(lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
    print_state("ST_ONE_TWO_THREE_SYM_LEX", linfo->c);
#endif

    if (buf->count_sym == 0) {
        add_to_buffer(buf, linfo->c);
        deferred_get_char(linfo);
    } else if (buf->count_sym == 1) {
        st_one_two_three_sym_lex_prev_c = get_last_from_buffer(buf);
        add_to_buffer(buf, linfo->c);
        deferred_get_char(linfo);
    } else if (buf->count_sym == 2) {
        /* TODO: make it more pretty */
        char prev_c = get_last_from_buffer(buf);
        clear_buffer(buf);
        switch (st_one_two_three_sym_lex_prev_c) {
		case '<':
            lex = (st_one_two_three_sym_lex_prev_c == prev_c && prev_c == linfo->c) ?
				make_lex(LEX_HERESTRING) :
				(st_one_two_three_sym_lex_prev_c == prev_c && prev_c != linfo->c) ?
				make_lex(LEX_HEREDOC) :
				make_lex(LEX_INPUT);
			break;
        default:
            fprintf(stderr, "Lexer: error (type 1) in ST_ONE_TWO_THREE_SYM_LEX;");
            print_state("ST_ONE_TWO_THREE_SYM_LEX", linfo->c);
            exit(ES_LEXER_INCURABLE_ERROR);
        }
        if (prev_c == linfo->c)
            deferred_get_char(linfo);
        linfo->state = ST_START;
        return lex;
    } else {
        fprintf(stderr, "Lexer: error (type 2) in ST_ONE_TWO_THREE_SYM_LEX;");
        print_state("ST_ONE_TWO_THREE_SYM_LEX", linfo->c);
        exit(ES_LEXER_INCURABLE_ERROR);
    }

    return lex;
}

lexeme *st_backslash(lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state("ST_BACKSLASH", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        return NULL;
    case '\n':
        /* Ignore newline symbol */
        print_prompt2();
        break;
/* Non bash-like behaviour. In bash substitution
 * makes in $'string' costruction. */
    case 'a':
        add_to_buffer(buf, '\a');
        break;
    case 'b':
        add_to_buffer(buf, '\b');
        break;
    case 'f':
        add_to_buffer(buf, '\f');
        break;
    case 'n':
        add_to_buffer(buf, '\n');
        break;
    case 'r':
        add_to_buffer(buf, '\r');
        break;
    case 't':
        add_to_buffer(buf, '\t');
        break;
    case 'v':
        add_to_buffer(buf, '\v');
        break;
    default:
        add_to_buffer(buf, linfo->c);
        break;
    }

    deferred_get_char(linfo);
    linfo->state = ST_WORD;

    return NULL;
}

lexeme *st_backslash_in_quotes(lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state("ST_BACKSLASH_IN_QUOTES", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        break;
    case '\"':
        add_to_buffer(buf, linfo->c);
        break;
    case '\n':
        print_prompt2();
        /* fallthrough */
    default:
        add_to_buffer(buf, '\\');
        add_to_buffer(buf, linfo->c);
        break;
    }
    deferred_get_char(linfo);
    linfo->state = ST_IN_QUOTES;

    return NULL;
}

lexeme *st_in_quotes(lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state("ST_IN_QUOTES", linfo->c);
#endif

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        return NULL;
    case '\\':
        deferred_get_char(linfo);
        linfo->state = ST_BACKSLASH_IN_QUOTES;
        break;
    case '\"':
        deferred_get_char(linfo);
        linfo->state = ST_WORD;
        break;
    case '\n':
        print_prompt2();
        /* fallthrough */
    default:
        add_to_buffer(buf, linfo->c);
        deferred_get_char(linfo);
        break;
    }

    return NULL;
}

lexeme *st_in_heredoc(lexer_info *linfo, buffer *buf)
{
#ifdef LEXER_DEBUG
    print_state("ST_IN_HEREDOC", linfo->c);
#endif
	add_to_buffer(buf, linfo->c);
	if (!buf->first_block) {
		str_mallocr(heredoc[0].contents, 1);
	}
	if (linfo->c != EOF) str_insert_char(heredoc[0].contents, linfo->c);
	if (linfo->c == '\n') str_undo(heredoc[0].contents);
	if (*(heredoc[0].contents.string+heredoc[0].contents.len-strlen(heredoc[0].id)-1) == '\n') {
		if (strcmp(heredoc[0].contents.string+heredoc[0].contents.len-strlen(heredoc[0].id), heredoc[0].id) == 0) {
			switch (linfo->c) {
			case '\n':
			case EOF:
				heredoc[0].accepting = 0;
				heredoc[0].final = 1;
				linfo->state = ST_WORD;
				int i;
				for (i = 0; i < strlen(heredoc[0].id)+1; i++) str_undo(heredoc[0].contents);
				return NULL;
			}
		} else if (linfo->c == '\n') str_insert_char(heredoc[0].contents, linfo->c);
	} else if (linfo->c == '\n') str_insert_char(heredoc[0].contents, linfo->c);

    switch (linfo->c) {
    case EOF:
        linfo->state = ST_ERROR;
        return NULL;
    default:
        deferred_get_char(linfo);
        break;
    }
    heredoc[0].last_char_before_that = heredoc[0].last_char;
    heredoc[0].last_char = linfo->c;
    return NULL;
}

lexeme *st_word(lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
    print_state("ST_WORD", linfo->c);
#endif
	if (heredoc) if (heredoc[0].accepting) {
		linfo->state = ST_IN_HEREDOC;
		return lex;
	}
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
        lex = make_lex(heredoc? (heredoc[0].final ? LEX_HEREDOC_WORD : LEX_WORD) : LEX_WORD);
        lex->str = heredoc? (heredoc[0].final ? heredoc[0].contents.string: convert_to_string(buf, 1)) : convert_to_string(buf, 1);
		if (heredoc) if (heredoc[0].final) {
			heredoc[0].final = 0;
			heredoc_remove();
		}

        return lex;
    case '\\':
        deferred_get_char(linfo);
        linfo->state = ST_BACKSLASH;
        break;
    case '\"':
        deferred_get_char(linfo);
        linfo->state = ST_IN_QUOTES; /* MARK */
        break;
    default:
        add_to_buffer(buf, linfo->c);
        deferred_get_char(linfo);
    }

    return lex;
}

lexeme *st_error(lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

    lex = make_lex(LEX_ERROR);
    print_state("ST_ERROR", linfo->c);
    clear_buffer(buf);
    linfo->get_next_char = 0;
    linfo->state = ST_START;
    /* TODO: read to '\n' or EOF (flush read buffer) */

    return lex;
}

lexeme *st_eoln_eof(lexer_info *linfo, buffer *buf)
{
    lexeme *lex = NULL;

#ifdef LEXER_DEBUG
    print_state("ST_EOLN_EOF", linfo->c);
#endif

    switch (linfo->c) {
    case '\n':
        lex = make_lex(LEX_EOLINE);
        break;
    case EOF:
        lex = make_lex(LEX_EOFILE);
        break;
    default:
        fprintf(stderr, "Lexer: error in ST_EOLN_EOF;");
        print_state("ST_EOLN_EOF", linfo->c);
        exit(ES_LEXER_INCURABLE_ERROR);
    }

    deferred_get_char(linfo);
    linfo->state = ST_START;

    return lex;
}

lexeme *get_lex(lexer_info *linfo)
{
    lexeme *lex = NULL;
    buffer buf;
	int do_once = 1;

    new_buffer(&buf);

    do {
        if (linfo->get_next_char) {
            linfo->get_next_char = 0;
            get_char(linfo);
        }
        if (linfo->state != ST_START) {
			if (do_once) {
				lexer_tokens.column_s = lexer_tokens.column;
				lexer_tokens.line_s = lexer_tokens.line;
			}
			do_once = 0;
		}

        switch (linfo->state) {
        case ST_START:
            lex = st_start(linfo, &buf);
            break;

        case ST_ONE_SYM_LEX:
            lex = st_one_sym_lex(linfo, &buf);
            break;

        case ST_ONE_TWO_SYM_LEX:
            lex = st_one_two_sym_lex(linfo, &buf);
            break;

        case ST_ONE_TWO_THREE_SYM_LEX:
            lex = st_one_two_three_sym_lex(linfo, &buf);
            break;
			
        case ST_BACKSLASH:
            lex = st_backslash(linfo, &buf);
            break;

        case ST_BACKSLASH_IN_QUOTES:
            lex = st_backslash_in_quotes(linfo, &buf);
            break;

        case ST_IN_QUOTES:
            lex = st_in_quotes(linfo, &buf); /* MARK */
            break;

        case ST_IN_HEREDOC:
            lex = st_in_heredoc(linfo, &buf); /* MARK */
            break;
			
        case ST_WORD:
            lex = st_word(linfo, &buf);
            break;

        case ST_ERROR:
            lex = st_error(linfo, &buf);
            break;

        case ST_EOLN_EOF:
            lex = st_eoln_eof(linfo, &buf);
            break;
        } /* switch */
    } while (lex == NULL);
	store_lex(lex);
    return lex;
}

/*
gcc -g3 -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g3 -Wall -ansi -pedantic -c utils.c -o utils.o &&
gcc -g3 -Wall -ansi -pedantic -c lexer.c -o lexer.o &&
gcc -g3 -Wall -ansi -pedantic -c parser.c -o parser.o &&
gcc -g3 -Wall -ansi -pedantic -c word_buffer.c -o word_buffer.o &&
gcc -g3 -Wall -ansi -pedantic lexer_.c -o lexer -DLEXER && ./lexer
gcc -g3 -Wall -ansi -pedantic parser_.c -o parser -DPARSER && ./parser

*/
#ifdef LEXER
int main()
{
    lexer_info linfo;
    init_lexer(&linfo);

    do {
        lexeme *lex;
#ifdef LEXER_DEBUG
		printf_b("\nSTART_OF_LEXING\n");
#endif
		lex = get_lex(&linfo);
#ifdef LEXER_DEBUG
		puts("printing lex list");
#endif
        print_lex(stdout, lex);
#ifdef LEXER_DEBUG
		puts("printed lex list");
#endif
        if (lex->type == LEX_EOFILE)
            return 0;
        destroy_lex(lex);

        if (linfo.state == ST_ERROR) {
            fprintf(stderr, "(>_<)\n");
            continue;
        }
    } while (1);
}
#endif
