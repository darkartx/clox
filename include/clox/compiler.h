#ifndef __CLOX_COMPILER_H__
#define __CLOX_COMPILER_H__

#include "chunk.h"

bool clox_compile(const char *source, clox_chunk *chunk);

#endif // __CLOX_COMPILER_H__
