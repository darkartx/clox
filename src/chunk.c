#include <stdlib.h>

#include "clox/chunk.h"
#include "memory.h"

void clox_init_chunk(clox_chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    clox_init_value_array(&chunk->constants);
}

void clox_free_chunk(clox_chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    clox_free_value_array(&chunk->constants);
    clox_init_chunk(chunk);
}

void clox_write_chunk(clox_chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int clox_chunk_add_constant(clox_chunk *chunk, clox_value value)
{
    clox_write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}
