#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"
#include "value.h"
#include "object.h"

#define CLOX_STACK_MAX 256

typedef struct {
    clox_chunk* chunk;
    uint8_t* ip;
    clox_value stack[CLOX_STACK_MAX];
    clox_value* stack_top;
    clox_obj* objects;
} clox_vm;

typedef enum {
    CLOX_INTERPRET_OK,
    CLOX_INTERPRET_COMPILE_ERROR,
    CLOX_INTERPRET_RUNTIME_ERROR
} clox_interpret_result;

extern clox_vm clox_vm_instance;

void clox_init_vm();
void clox_free_vm();
clox_interpret_result clox_interpret(const char *source);
void clox_stack_push(clox_value value);
clox_value clox_stack_pop();
clox_value clox_stack_peek(int distance);

#endif // __CLOX_VM_H__
