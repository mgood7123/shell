#ifndef _WORD_BUFFER_H
#define _WORD_BUFFER_H

typedef struct word_item {
	struct word_item *next;
	char *str;
} word_item;

typedef struct word_buffer {
    word_item *first_item;
    word_item *last_item;
    unsigned int count_words;
} word_buffer;

void new_word_buffer (word_buffer *wbuf);
void add_to_word_buffer (word_buffer *wbuf, char *str);
void clear_word_buffer (word_buffer *wbuf, int destroy_str);
char **convert_to_argv (word_buffer *wbuf, int destroy_me);
char *get_last_word (word_buffer *wbuf);
void destroy_argv (char **argv);

#endif
