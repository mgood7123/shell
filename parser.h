#ifndef _PARSER_H
#define _PARSER_H

#include "strlist.h"
#include "common.h"

typedef struct rState {
	int p;
	int c;
	unsigned int eof:1;
	unsigned int eoln:1;
	unsigned int eow:1;
	unsigned int in_quotes:1;
	unsigned int error:1;
/* for free() */
	unsigned int str_need:1;
/* temporally, for get_next_token() */
	strL *cur_item;
	char *cur_sym;
	int count_sym;
/* for make commands */
	char *cur_token;
	int token_type;
} rState;


rState *new_rState(void);
void get_next_sym(rState *s);
command *get_command(rState *s);
void print_command(command *head);
void dispose_command(command *cmd);
void unset_error(rState *s);

#endif
