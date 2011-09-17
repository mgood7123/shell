#define BLOCK_SIZE 8
#define REAL_BLOCK_SIZE (BLOCK_SIZE+1)

// Работа только со строками, разбитыми на блоки, размером BLOCK_SIZE.

typedef struct buffer {
    strblock *first_block;
    strblock *last_block;
    unsigned int count_sym;
} buffer;
typedef struct strblock {
	char *str;
	struct strblock *next;
} strblock;

new()
add(char)
// need? remove_last()
// need? extract_last(int n)// выделить последние n символов в отдельный buffer.
flush/destroy()
convert_to_string(destroy_me)
