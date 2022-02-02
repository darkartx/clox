#ifndef __CLOX_COMPILER_H__
#define __CLOX_COMPILER_H__

#include "chunk.h"
#include "object.h"

clox_obj_function* clox_compile(const char *source);

#endif // __CLOX_COMPILER_H__
