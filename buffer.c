#include "buffer.h"

#include <stdlib.h>
#include <string.h>

void new_buffer (buffer *buf)
{
/*  buffer *buf = (buffer *) malloc (sizeof (buffer)); */
    buf->first_block = NULL;
    buf->last_block = NULL;
    buf->count_sym = 0;
}

void add_to_buffer (buffer *buf, char c)
{
    int pos = buf->count_sym % BLOCK_SIZE;

    if (buf->first_block == NULL) {
        buf->last_block = buf->first_block =
            (strblock *) malloc (sizeof (strblock));
        buf->last_block->next = NULL;
        buf->last_block->str =
            (char *) malloc (sizeof (char) * BLOCK_SIZE);
    } else if (pos == 0) {
        buf->last_block = buf->last_block->next =
            (strblock *) malloc (sizeof (strblock));
        buf->last_block->next = NULL;
        buf->last_block->str =
            (char *) malloc (sizeof (char) * BLOCK_SIZE);
    }

    *(buf->last_block->str + pos) = c;
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
    int cur_size;

    while (current != NULL) {
        next = current->next;
        if (next == NULL)
            cur_size = buf->count_sym + str - cur_sym_to;
        else
            cur_size = BLOCK_SIZE;
        memcpy (cur_sym_to, current->str, cur_size);
        cur_sym_to += cur_size;
        if (destroy_me) {
            free (current->str);
            free (current);
        }
        current = next;
    }

    *cur_sym_to = '\0';
    return str;
}

/*
#include <stdio.h>
int main() {
    int c;
    char *str;
    buffer buf;
    new_buffer (&buf);
    do {
        do {
            c = getchar ();
            if (c == EOF || c == '\n')
                break;
            add_to_buffer (&buf, c);
        } while (1);
        str = convert_to_string (&buf, 0);
        printf ("[%s]\n", str);
        free (str);
        clear_buffer (&buf);
    } while (c != EOF);
    return 0;
}
*/
