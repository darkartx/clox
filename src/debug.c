#include <stdlib.h>
#include <stdio.h>

#include "clox/debug.h"
#include "clox/value.h"

static int simple_instruction(const char* name, int offset);
static int constant_instruction(const char* name, clox_chunk* chunk, int offset);
static int byte_instruction(const char* name, clox_chunk* chunk, int offset);
static int jump_instruction(const char* name, int sign, clox_chunk* chunk, int offset);

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
    case CLOX_OP_ADD:
        return simple_instruction("opAdd", offset);
    case CLOX_OP_SUBTRACT:
        return simple_instruction("opSubtract", offset);
    case CLOX_OP_MULTIPLY:
        return simple_instruction("opMultiply", offset);
    case CLOX_OP_DEVIDE:
        return simple_instruction("opDevide", offset);
    case CLOX_OP_NEGATE:
        return simple_instruction("opNegate", offset);
    case CLOX_OP_NIL:
        return simple_instruction("opNil", offset);
    case CLOX_OP_TRUE:
        return simple_instruction("opTrue", offset);
    case CLOX_OP_FALSE:
        return simple_instruction("opFalse", offset);
    case CLOX_OP_NOT:
        return simple_instruction("opNot", offset);
    case CLOX_OP_EQUAL:
        return simple_instruction("opEqual", offset);
    case CLOX_OP_GREATER:
        return simple_instruction("opGreater", offset);
    case CLOX_OP_LESS:
        return simple_instruction("opLess", offset);
    case CLOX_OP_PRINT:
        return simple_instruction("opPrint", offset);
    case CLOX_OP_POP:
        return simple_instruction("opPop", offset);
    case CLOX_OP_DEFINE_GLOBAL:
        return constant_instruction("opDefineGlobal", chunk, offset);
    case CLOX_OP_GET_GLOBAL:
        return constant_instruction("opGetGlobal", chunk, offset);
    case CLOX_OP_SET_GLOBAL:
        return constant_instruction("opSetGlobal", chunk, offset);
    case CLOX_OP_GET_LOCAL:
        return byte_instruction("opGetLocal", chunk, offset);
    case CLOX_OP_SET_LOCAL:
        return byte_instruction("opSetLocal", chunk, offset);
    case CLOX_OP_JUMP_IF_FALSE:
        return jump_instruction("opJumpIfFalse", 1, chunk, offset);
    case CLOX_OP_JUMP:
        return jump_instruction("opJump", 1, chunk, offset);
    case CLOX_OP_LOOP:
        return jump_instruction("opLoop", -1, chunk, offset);
    case CLOX_OP_CALL:
        return byte_instruction("opCall", chunk, offset);
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

static int byte_instruction(const char* name, clox_chunk* chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jump_instruction(const char* name, int sign, clox_chunk* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}
