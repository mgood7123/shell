#include "buffer.h"

buffer *new_buffer ()
{
    buffer *buf = (buffer *) malloc (sizeof (strlist));
    buf->first_block = NULL;
    buf->last_block = NULL;
    buf->count_sym = 0;
    return buf;
}

void add_to_buffer (buffer *buf, char c)
{
    int pos = buf->count_sym % BLOCK_SIZE;

    if (buf->first_block == NULL) {
        buf->last_block = buf->first_block =
            (strblock *) malloc (sizeof (strblock);
        buf->last_block->str =
            (char *) malloc (sizeof (char) * BLOCK_SIZE);
    } else if (pos == 0) {
        buf->last_block = buf->last_block->next =
            (strblock *) malloc (sizeof (strblock));
        buf->last_block->str =
            (char *) malloc (sizeof (char) * BLOCK_SIZE);
    }

    *(buf->last_block + pos) = c;
    ++(buf->count_sym);
}

void clear_buffer (buffer *buf)
{
    strblock *current = buf->first_block;
    strblock *next;

    while (current != NULL) {
        next = current->next;
        free (current->str);
        free (current);
        current = next;
    }

    buf->last_block = buf->first_block = NULL;
    buf->count_sym = 0;
}

char *convert_to_string (buffer *buf, int destroy_me)
{
    strblock *current = buf->first_block;
    strblock *next;
    char *str = (char *) malloc 
        (sizeof (char) * (buf->count_sym + 1));
    char *cur_sym_to = str;

    while (current != NULL) {
        next = current->next;
        memcpy(cur_sym_to, current->str, BLOCK_SIZE);
        cur_sym_to += BLOCK_SIZE;
        if (destroy_me) {
            free (current->str);
            free (current);
        }
        current = next;
    }

    cur_sym_to = '\0';
    return str;
}
