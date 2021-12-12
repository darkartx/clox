#ifndef __CLOX_CHUNK_H__
#define __CLOX_CHUNK_H__

#include "common.h"
#include "value.h"

typedef enum {
    CLOX_OP_RETURN,
    CLOX_OP_CONSTANT,
} clox_op_code;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    clox_value_array constants;
    int *lines;
} clox_chunk;

void clox_init_chunk(clox_chunk *chunk);
void clox_free_chunk(clox_chunk *chunk);
void clox_write_chunk(clox_chunk *chunk, uint8_t byte, int line);

int clox_chunk_add_constant(clox_chunk *chunk, clox_value value);

#endif // __CLOX_CHUNK_H__
