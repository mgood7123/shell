#ifndef _STRLIST_H
#define _STRLIST_H

typedef struct strlist {
	char *str;
	struct strlist *next;
} strlist;

strlist *new_strlist(int block_size);
void add_strlist(strlist **list, char *str);
char **strlist_to_vector(strlist *list, int count,
		int dispose_structure);
void dispose_strlist(strlist *list, int dispose_string);
char *strlist_to_str(strlist *list, int block_size,
		int count_sym, int dispose_structure);

#endif
