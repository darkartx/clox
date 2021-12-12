#ifndef __CLOX_DEBUG_H__
#define __CLOX_DEBUG_H__

#include "chunk.h"

void clox_disassemble_chunk(clox_chunk *chunk, const char *name);
int clox_disassemble_instruction(clox_chunk *chunk, int offset);

#endif // __CLOX_DEBUG_H__
