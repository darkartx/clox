#include <stdlib.h>
#include <stdio.h>

#include "clox/debug.h"
#include "clox/value.h"

void clox_disassemble_chunk(clox_chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = clox_disassemble_instruction(chunk, offset);
    }
}

int clox_disassemble_instruction(clox_chunk *chunk, int offset)
{
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
    case CLOX_OP_RETURN:
        return simple_instruction("opReturn", offset);
    case CLOX_OP_CONSTANT:
        return constant_instruction("opConstant", chunk, offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

static int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int constant_instruction(const char *name, clox_chunk *chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %04d '", name, constant);
    clox_print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}


