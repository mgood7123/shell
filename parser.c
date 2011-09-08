#include "parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BLOCK_SIZE 8
#define REAL_BLOCK_SIZE (BLOCK_SIZE+1)

#define TOKEN_NULL -1
#define TOKEN_OTHER 0
#define TOKEN_INPUT 1
#define TOKEN_OUTPUT 2
#define TOKEN_APPEND 3
#define TOKEN_PIPELINE 4
#define TOKEN_AND 5
#define TOKEN_OR 6
#define TOKEN_SEMICOLON 7
#define TOKEN_OPEN 8
#define TOKEN_CLOSE 9
#define TOKEN_REVERSE 10
#define TOKEN_BG 11

#define if_not_null_free(pointer); \
do {\
	if (pointer) {\
		free(pointer);\
		pointer = NULL;\
	} \
} while (0);

void print_prompt2(void);
void dispose_command(command *cmd);

rState *new_rState(void)
{
	rState *s = (rState *) malloc(sizeof(rState));
	s->p = '\0';
	s->c = '\0';
	s->eof = 0;
	s->eoln = 0;
	s->eow = 0;
	s->in_quotes = 0;
	s->error = 0;
	s->str_need = 0;
	s->cur_item = NULL;
	s->cur_sym = NULL;
	s->count_sym = 0;
	s->cur_token = NULL;
	return s;
}

int get_escaped_sym(char c)
{
	switch (c) {
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	default:
		return c;
	}
}

void set_error(rState *s)
{
	/* set error flag && break all loops */
	s->error = s->eoln = s->eow = 1;
	if_not_null_free(s->cur_token);
	s->token_type = TOKEN_NULL;
}

void unset_error(rState *s)
{
	s->error = 0;
}

void set_error_to(rState *s, int condition)
{
	if (condition)
		set_error(s);
	else
		unset_error(s);
}

void change_quoting(rState *s)
{
	set_error_to(s, s->eof);

	if (s->c == '\n') {
		print_prompt2();
		s->c = '\0';
	}
	if (s->p == '\\' && (!s->in_quotes || s->c == '\"')) {
		s->p = get_escaped_sym(s->c);
		s->c = '\0';
	}
}

void check_for_token(rState *s, char ch, int several_char)
{
	if (s->p == ch || s->c == ch)
		s->eow = several_char ? !(s->p == ch && s->c == ch) : 1;
	if (several_char ? (s->p == ch && s->c != ch) : (s->p == ch))
		s->token_type = TOKEN_NULL; /* type will be assigned later */
}

void change_unquoting(rState *s)
{
	s->eoln = s->eof || (s->c == '\n');
	s->eow = s->eoln || (s->c == ' ');

	check_for_token(s, '&', 1);
	check_for_token(s, '|', 1);
	check_for_token(s, '>', 1);
	check_for_token(s, '<', 0);
	check_for_token(s, ';', 0);
	check_for_token(s, '(', 0);
	check_for_token(s, ')', 0);
	check_for_token(s, '`', 0);
}

void change_rState(rState *s)
{
	s->eof = (s->c == EOF);

	if (s->p == '\"') {
		s->in_quotes = !s->in_quotes;
		s->p = '\0';
	}

	if (s->in_quotes || s->p == '\\')
		change_quoting(s);
	else
		change_unquoting(s);
}

void get_next_sym(rState *s)
{
	s->p = s->c;
	s->c = getchar();
	change_rState(s);
}

void skip_spaces(rState *s)
{
	while (s->c == ' ' && !s->eoln)
		get_next_sym(s);
}

void add_sym(rState *s)
{
	if ((s->cur_sym - s->cur_item->str) == (REAL_BLOCK_SIZE - 1)) {
		*(s->cur_sym) = '\0';
		s->cur_item = s->cur_item->next = new_strL(REAL_BLOCK_SIZE);
		s->cur_sym = s->cur_item->str;
	}
	*(s->cur_sym) = s->p;
	++(s->cur_sym);
	++(s->count_sym);
}

int token_type(char *str)
{
	if (str == NULL)
		return TOKEN_NULL;
	if (!strcmp(str, "<"))
		return TOKEN_INPUT;
	if (!strcmp(str, ">"))
		return TOKEN_OUTPUT;
	if (!strcmp(str, ">>"))
		return TOKEN_APPEND;
	if (!strcmp(str, "|"))
		return TOKEN_PIPELINE;
	if (!strcmp(str, "&&"))
		return TOKEN_AND;
	if (!strcmp(str, "||"))
		return TOKEN_OR;
	if (!strcmp(str, ";"))
		return TOKEN_SEMICOLON;
	if (!strcmp(str, "("))
		return TOKEN_OPEN;
	if (!strcmp(str, ")"))
		return TOKEN_CLOSE;
	if (!strcmp(str, "`"))
		return TOKEN_REVERSE;
	if (!strcmp(str, "&"))
		return TOKEN_BG;
	return TOKEN_OTHER;
}

void get_next_token(rState *s)
{
	strL *list = NULL;

	if (!s->str_need && s->cur_token)
		free(s->cur_token);

	s->count_sym = 1; /* 0 + one for '\0' */
	s->cur_token = NULL;
	s->token_type = TOKEN_OTHER;
	s->str_need = 0;

	skip_spaces(s);
	if (s->eoln) {
		s->token_type = TOKEN_NULL;
		return;
	}

	list = s->cur_item = new_strL(REAL_BLOCK_SIZE);
	s->cur_sym = s->cur_item->str;

	do {
		get_next_sym(s);
		if (s->p != '\0')
			add_sym(s);
	} while (!s->eow);
	*(s->cur_sym) = '\0';

	if (s->error) {
		dispose_strL(list, 1);
		set_error(s);
		return;
	}

	s->cur_token = strL_to_str(list, BLOCK_SIZE, s->count_sym, 1);
	if (s->token_type == TOKEN_NULL)
		s->token_type = token_type(s->cur_token);
}

pipeline *new_pipeline_node(void)
{
	pipeline *pl =
		(pipeline *) malloc(sizeof(pipeline));

	pl->argv = NULL;
	pl->input = NULL;
	pl->output = NULL;
	pl->append = 0;
	pl->next = NULL;

	return pl;
}

void dispose_vector(char **vector)
{
	char **cur_str = vector;

	if (vector == NULL)
		return;

	while (*cur_str)
		free(*(cur_str++));
	free(vector);
}

void dispose_pipeline(pipeline *pl)
{
	pipeline *next;

	while (pl) {
		next = pl->next;
		dispose_vector(pl->argv);
		if_not_null_free(pl->input);
		if_not_null_free(pl->output);
		free(pl);
		pl = next;
	}
}

void change_in_out(rState *s, pipeline *scmd,
			int input, int append)
{
	get_next_token(s);
	if (s->error)
		return;

	if (s->token_type == TOKEN_OTHER) {
		if (input) {
			scmd->input = s->cur_token;
		} else {
			scmd->output = s->cur_token;
			scmd->append = append;
		}
		s->str_need = 1;
	} else {
		set_error(s);
	}
}

/* Type of current token must be TOKEN_OTHER */
pipeline *get_simple_command(rState *s)
{
	pipeline *scmd = new_pipeline_node();
	strL *to_argv = NULL;
	int count = 0;

	while (!s->error) {
		if (s->token_type == TOKEN_OTHER) {
			add_strL(&to_argv, s->cur_token);
			s->str_need = 1;
			++count;
		} else if (s->token_type == TOKEN_INPUT
				|| s->token_type == TOKEN_OUTPUT
				|| s->token_type == TOKEN_APPEND) {
			change_in_out(s, scmd,
				s->token_type == TOKEN_INPUT,
				s->token_type == TOKEN_APPEND);
			if (s->error)
				break;
		} else
			break;

		get_next_token(s);
	}

	if (s->error) {
		dispose_strL(to_argv, 1);
		dispose_pipeline(scmd);
		return NULL;
	}

	scmd->argv = strL_to_vector(to_argv, count, 1);
	scmd->next = NULL;

	return scmd;
}

/* Type of current token must be TOKEN_OTHER */
pipeline *get_pipeline(rState *s)
{
	pipeline *pl = get_simple_command(s), *cur_cmd = pl;

	while (s->token_type == TOKEN_PIPELINE) {
		get_next_token(s);
		cur_cmd = cur_cmd->next = get_simple_command(s);
	}

	if (s->error) {
		dispose_pipeline(pl);
		return NULL;
	}

	return pl;
}

command *new_cmds_head(void)
{
	command *head = (command *)
		malloc(sizeof(command));

	head->list = NULL;
	head->background = 0;

	return head;
}

commands_list *new_cmds_node(void)
{
	commands_list *pl =
		(commands_list *) malloc(sizeof(commands_list));

	pl->pl = NULL;
	pl->cmds = NULL;
	pl->relation = TOKEN_OTHER;
	pl->next = NULL;

	return pl;
}

void next_cmds_node(command *cmds,
			commands_list **cur_pl)
{
	if (cmds->list)
		*cur_pl = (*cur_pl)->next = new_cmds_node();
	else
		cmds->list = *cur_pl = new_cmds_node();
}

/* *cmds != NULL */
void cur_cmds_to_child(command **cmds,
			commands_list **cur_pl)
{
	command *parent = new_cmds_head();
	parent->list = new_cmds_node();
	parent->list->cmds = *cmds;
	*cur_pl = parent->list;
	*cmds = parent;
}

void dispose_commands_list(commands_list *list)
{
	commands_list *next;

	if (list == NULL)
		return;

	while (list) {
		next = list->next;
		dispose_pipeline(list->pl);
		dispose_command(list->cmds);
		free(list);
		list = next;
	}
}

void dispose_command(command *cmd)
{
	if (cmd == NULL)
		return;

	dispose_commands_list(cmd->list);
	free(cmd);
}

command *get_command(rState *s)
{
	command *head = new_cmds_head();
	commands_list *cur_pl = NULL;

	do {
		get_next_token(s);
		if (s->error)
			break;

#ifdef DEBUG_PARSER
		fprintf(stderr, "1_DEBUG: \"%s\" - %d\n",
				s->cur_token, s->token_type);
#endif

		if (head->background && (s->token_type != TOKEN_NULL
				&& s->token_type != TOKEN_CLOSE)) {
			set_error(s);
			break;
		}

		switch (s->token_type) {
		case TOKEN_NULL:
			return head;
		case TOKEN_OTHER:
			next_cmds_node(head, &cur_pl);
			cur_pl->pl = get_pipeline(s);
			break;
		case TOKEN_OPEN:
			next_cmds_node(head, &cur_pl);
			cur_pl->cmds = get_command(s);
			if (s->token_type == TOKEN_CLOSE)
				get_next_token(s);
			else
				set_error(s);
			break;
		case TOKEN_REVERSE:
			/* not implemented */
			set_error(s);
			break;
		default:
			set_error(s);
			break;
		}

		if (s->error)
			break;

#ifdef DEBUG_PARSER
		fprintf(stderr, "2_DEBUG: \"%s\" - %d\n",
				s->cur_token, s->token_type);
#endif

		switch (s->token_type) {
		case TOKEN_NULL:
		case TOKEN_CLOSE:
			return head;
		case TOKEN_SEMICOLON:
			/* lists1 ; lists2 ::= (lists1) ; (lists2) */
			cur_cmds_to_child(&head, &cur_pl);
			cur_pl->relation = s->token_type;
			next_cmds_node(head, &cur_pl);
			cur_pl->cmds = get_command(s);
			break;
		case TOKEN_AND:
		case TOKEN_OR:
			cur_pl->relation = s->token_type;
			break;
		case TOKEN_BG:
			if (head->background)
				set_error(s); /* non bash-like behavior */
			else
				head->background = 1;
			break;
		default:
			set_error(s);
			break;
		}
	} while (!s->eoln && !s->error);

	if (s->error) {
		dispose_command(head);
		return NULL;
	}

	free(s->cur_token);
	s->cur_token = NULL;

	return head;
}

void print_relation(int relation)
{
	switch (relation) {
	case TOKEN_NULL:
		printf("%s", "TOKEN_NULL");
		break;
	case TOKEN_INPUT:
		printf("%s", "<");
		break;
	case TOKEN_OUTPUT:
		printf("%s", ">");
		break;
	case TOKEN_APPEND:
		printf("%s", ">>");
		break;
	case TOKEN_PIPELINE:
		printf("%s", "|");
		break;
	case TOKEN_AND:
		printf("%s", "&&");
		break;
	case TOKEN_OR:
		printf("%s", "||");
		break;
	case TOKEN_SEMICOLON:
		printf("%s", ";");
		break;
	case TOKEN_OPEN:
		printf("%s", "(");
		break;
	case TOKEN_CLOSE:
		printf("%s", ")");
		break;
	case TOKEN_REVERSE:
		printf("%s", "`");
		break;
	case TOKEN_BG:
		printf("%s", "TOKEN_BG");
		break;
	default:
		printf("%s", "TOKEN_OTHER");
		break;
	}
	printf("%s", " ");
}

void print_pipeline(pipeline *pl)
{
	do {
		char **argv2 = pl->argv;
		while (*argv2)
			printf("%s ", *(argv2++));

		if (pl->input)
			printf("< %s ", pl->input);
		if (pl->output && !pl->append)
			printf("> %s ", pl->output);
		if (pl->output && pl->append)
			printf(">> %s ", pl->output);

		pl = pl->next;
		if (pl)
			printf("%s ", "|");
	} while (pl);
}

void print_command(command *head)
{
	commands_list *cmds = head->list;

	if (head->background)
		printf("%s ", "IN_BG:");

	printf("%s ", "(");
	while (cmds) {
		if (cmds->pl)
			print_pipeline(cmds->pl);
		else
			print_command(cmds->cmds);
		if (cmds->relation != TOKEN_OTHER)
			print_relation(cmds->relation);
		cmds = cmds->next;
	}
	printf("%s ", ")");
}
