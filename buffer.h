#ifndef _BUFFER_H
#define _BUFFER_H

#define BLOCK_SIZE 8

typedef struct strblock {
	char *str;
	struct strblock *next;
} strblock;

typedef struct buffer {
    strblock *first_block;
    strblock *last_block;
    unsigned int count_sym;
} buffer;

buffer *new_buffer ();
void add_to_buffer (buffer *buf, char c);
void clear_buffer (buffer *buf);
char *convert_to_string (buffer *buf, int destroy_me);

#endif
