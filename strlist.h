#ifndef _STRLIST_H
#define _STRLIST_H

typedef struct strL {
	char *str;
	struct strL *next;
} strL;

strL *new_strL(int block_size);
void add_strL(strL **list, char *str);
char **strL_to_vector(strL *list, int count,
		int dispose_structure);
void dispose_strL(strL *list, int dispose_string);
char *strL_to_str(strL *list, int block_size,
		int count_sym, int dispose_structure);

#endif
