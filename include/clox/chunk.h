#ifndef __CLOX_CHUNK_H__
#define __CLOX_CHUNK_H__

#include "common.h"
#include "value.h"

typedef enum {
    CLOX_OP_RETURN,
    CLOX_OP_ADD,
    CLOX_OP_SUBTRACT,
    CLOX_OP_MULTIPLY,
    CLOX_OP_DEVIDE,
    CLOX_OP_NEGATE,
    CLOX_OP_CONSTANT,
    CLOX_OP_NIL,
    CLOX_OP_TRUE,
    CLOX_OP_FALSE,
    CLOX_OP_NOT,
    CLOX_OP_EQUAL,
    CLOX_OP_GREATER,
    CLOX_OP_LESS,
    CLOX_OP_PRINT,
    CLOX_OP_POP,
    CLOX_OP_DEFINE_GLOBAL,
    CLOX_OP_GET_GLOBAL,
    CLOX_OP_SET_GLOBAL,
    CLOX_OP_GET_LOCAL,
    CLOX_OP_SET_LOCAL
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
