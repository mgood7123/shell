#ifndef BUFFER_H_SENTRY
#define BUFFER_H_SENTRY

#ifndef BUFFER_BLOCK_SIZE
#define BUFFER_BLOCK_SIZE 8
#endif

typedef struct strblock {
	struct strblock *next;
	char *str;
} strblock;

typedef struct buffer {
    strblock *first_block;
    strblock *last_block;
    unsigned int count_sym;
} buffer;

void new_buffer (buffer *buf);
void add_to_buffer (buffer *buf, char c);
void clear_buffer (buffer *buf);
char *convert_to_string (buffer *buf, int destroy_me);
int get_last_from_buffer (buffer *buf);

#endif
