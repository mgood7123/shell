#include "strlist.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *new_str_block(int block_size)
{
	if (block_size > 0)
		return (char *) malloc(sizeof(char) * block_size);
	else
		return NULL;
}

strlist *new_strlist(int block_size)
{
	strlist *list = (strlist *) malloc(sizeof(strlist));
	list->str = new_str_block(block_size);
	list->next = NULL;
	return list;
}

void add_strlist(strlist **list, char *str)
{
	strlist *cur_item = *list;

	if (str == NULL)
		return;

	if (*list == NULL) {
		*list = new_strlist(-1);
		(*list)->str = str;
		return;
	}

	while (cur_item->next)
		cur_item = cur_item->next;
	cur_item->next = new_strlist(-1);
	cur_item->next->str = str;
}

/*
int items_count(strlist *list)
{
	strlist *cur_item = list;
	int count = 0;

	while (cur_item) {
		++count;
		cur_item = cur_item->next;
	}

	return count;
}
*/

char **strlist_to_vector(strlist *list,
		int count, int dispose_structure)
{
	strlist *cur_item = list;
	char **vlist = (char **) malloc((count + 1) * sizeof(char *));
	char **cur_vlist_item = vlist;
	strlist *next;
	
	while (cur_item && count > 0) {
		next = cur_item->next;
		*cur_vlist_item = cur_item->str;
		if (dispose_structure)
			free(cur_item);
		cur_item = next;
		++cur_vlist_item;
		--count;
	}

	*cur_vlist_item = NULL;
	if (dispose_structure)
		dispose_strlist(cur_item, 1);

	return vlist;
}

/*
void print_strlist(strlist *list)
{
	strlist *cur_item = list;

	while (cur_item) {
		printf("[%s]\n", cur_item->str);
		cur_item = cur_item->next;
	}
}
*/

void dispose_strlist(strlist *list, int dispose_string)
{
	strlist *next;
	while (list) {
		next = list->next;
		if (dispose_string)
			free(list->str);
		free(list);
		list = next;
	}
}

/*
int length_strlist(strlist *list, int block_size)
{
	strlist *cur_item = list;
	int count = 0;

	if (cur_item == NULL)
		return 0;

	while (cur_item->next) {
		if (block_size < 0)
			block_size = strlen(cur_item->str);
		count += block_size;
		cur_item = cur_item->next;
	}

	count += strlen(cur_item->str);
	return count;
}
*/

char *strlist_to_str(strlist *list, int block_size,
		int count_sym, int dispose_structure)
{
	strlist *cur_item = list;
	char *str = NULL, *cur_sym = NULL;

	if (list == NULL)
		return str;

	cur_sym = str = (char *) malloc(sizeof(char) * count_sym);

	while (cur_item->next) {
		strlist *tmp = cur_item;
		if (block_size < 0) /* for strings different length */
			block_size = strlen(cur_item->str);
		memcpy(cur_sym, cur_item->str, block_size);
		cur_sym += block_size;
		count_sym -= block_size;
		cur_item = cur_item->next;
		if (dispose_structure) {
			free(tmp->str);
			free(tmp);
		}
	}

	memcpy(cur_sym, cur_item->str, count_sym);
	if (dispose_structure) {
		free(cur_item->str);
		free(cur_item);
	}

	return str;
}
