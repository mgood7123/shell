#include "word_buffer.h"

#include <stdlib.h>

void new_word_buffer (word_buffer *wbuf)
{
/*  word_buffer *wbuf = (word_buffer *) malloc (sizeof (word_buffer)); */
    wbuf->first_item = NULL;
    wbuf->last_item = NULL;
    wbuf->count_words = 0;
}

void add_to_word_buffer (word_buffer *wbuf, char *str)
{
    if (wbuf->first_item == NULL) {
        wbuf->last_item = wbuf->first_item =
            (word_item *) malloc (sizeof (word_item));
    } else {
        wbuf->last_item = wbuf->last_item->next =
            (word_item *) malloc (sizeof (word_item));
    }

    wbuf->last_item->next = NULL;
    wbuf->last_item->str = str;
    ++(wbuf->count_words);
}

void clear_word_buffer (word_buffer *wbuf, int destroy_str)
{
    word_item *current = wbuf->first_item;
    word_item *next;

    while (current != NULL) {
        next = current->next;
        if (destroy_str)
            free (current->str);
        free (current);
        current = next;
    }

    wbuf->last_item = wbuf->first_item = NULL;
    wbuf->count_words = 0;
}

char **convert_to_argv (word_buffer *wbuf, int destroy_me)
{
    word_item *current = wbuf->first_item;
    word_item *next;
    char **argv = (char **) malloc (sizeof (char *)
        * (wbuf->count_words + 1));
    char **cur_str = argv;

    while (current != NULL) {
        next = current->next;
        *cur_str = current->str;
        if (destroy_me)
            free (current);
        ++cur_str;
        current = next;
    }

    if (destroy_me) {
        wbuf->last_item = wbuf->first_item = NULL;
        wbuf->count_words = 0;
    }

    *cur_str = NULL;
    return argv;
}

/* Returns NULL, if word_buffer empty */
char *get_last_word (word_buffer *wbuf)
{
    if (wbuf->last_item == NULL)
        return NULL;
    return wbuf->last_item->str;
}

void destroy_argv (char **argv)
{
    char **cur_str = argv;

    if (argv == NULL)
        return;

    while (*cur_str) {
        free(*cur_str);
        ++cur_str;
    }

    free (argv);
}

void print_argv (FILE *stream, char **argv)
{
    if (argv == NULL)
        return;

    while (*argv != NULL) {
        fprintf (stream, "[%s]", *argv);
        if (*(++argv) != NULL)
            fprintf (stream, " ");
    }
}

/*
gcc -g -Wall -ansi -pedantic word_buffer.c -o word_buffer
*/

/*
#define DEBUG_WORD_BUFFER_VAR 1
int main (int argc, char **argv) {
    char **cur_argv;
    char **my_argv;
    word_buffer wbuf;

    do {
        new_word_buffer (&wbuf);
        cur_argv = argv;

        while (*cur_argv != NULL) {
            add_to_word_buffer (&wbuf, *cur_argv);
            ++cur_argv;
        }

#if (DEBUG_WORD_BUFFER_VAR == 1)
        my_argv = convert_to_argv (&wbuf, 0);
        print_argv (my_argv);
        printf ("Last string: [%s]\n", get_last_word (&wbuf));
        / * Use it, only if strings in dynamic memory
        clear_word_buffer (&wbuf, 1); * /
        clear_word_buffer (&wbuf, 0);
        free (my_argv);
#elif (DEBUG_WORD_BUFFER_VAR == 2)
        my_argv = convert_to_argv (&wbuf, 0);
        print_argv (my_argv);
        printf ("Last string: [%s]\n", get_last_word (&wbuf));
        clear_word_buffer (&wbuf, 0);
        / * Use it, only if strings in dynamic memory
        destroy_argv (my_argv); * /
        free (my_argv);
#else
        my_argv = convert_to_argv (&wbuf, 1);
        print_argv (my_argv);
        / * Use it, only if strings in dynamic memory
        destroy_argv (my_argv); * /
        free (my_argv);
#endif
    } while (1);

    return 0;
}
*/
