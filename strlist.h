#ifndef _STRLIST_H
#define _STRLIST_H

typedef struct strlist {
	char *str;
	struct strlist *next;
} strlist;

typedef struct strlist_ext {
    strlist *strlist;
    strlist *last_item;
    unsigned int count_sym;
    unsigned int block_size;
} strlist_ext;

strlist *new_strlist(unsigned int block_size);
void add_to_strlist(strlist **list, char *str);
char **strlist_to_vector(strlist *list, int count,
		int dispose_structure);
void dispose_strlist(strlist *list, int dispose_string);
char *strlist_to_str(strlist *list, unsigned int block_size,
		int count_sym, int dispose_structure);

strlist_ext *new_strlist_ext (unsigned int block_size);
strlist *add_item_to_strlist_ext(strlist_ext *e)

#endif
