#include <stdio.h>

#include "clox/common.h"
#include "clox/vm.h"
#include "clox/debug.h"
#include "clox/compiler.h"

clox_vm vm;

static void reset_stack();
static clox_interpret_result run();

void clox_init_vm()
{
    reset_stack();
}

void clox_free_vm()
{
}

clox_interpret_result clox_interpret(const char *source)
{
    clox_chunk chunk;
    clox_init_chunk(&chunk);

    if (!clox_compile(source, &chunk)) {
        clox_free_chunk(&chunk);
        return CLOX_INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    clox_interpret_result result = run();

    clox_free_chunk(&chunk);
    return result;
}

void clox_stack_push(clox_value value)
{
    *vm.stack_top = value;
    vm.stack_top++;
}

clox_value clox_stack_pop()
{
    vm.stack_top--;
    return *vm.stack_top;
}

static clox_interpret_result run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
        double b = clox_stack_pop(); \
        double a = clox_stack_pop(); \
        clox_stack_push(a op b); \
    } while (false)

    for (;;) {
#ifdef CLOX_DEBUG_TRACE_EXECUTION
        printf("      ");
        for (clox_value *slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            clox_print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        clox_disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case CLOX_OP_CONSTANT: {
                clox_value constant = READ_CONSTANT();
                clox_stack_push(constant);
                break;
            }
            case CLOX_OP_ADD: {
                BINARY_OP(+);
                break;
            }
            case CLOX_OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            case CLOX_OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            case CLOX_OP_DEVIDE: {
                BINARY_OP(/);
                break;
            }
            case CLOX_OP_NEGATE: {
                clox_stack_push(
                    -clox_stack_pop()
                );
                break;
            }
            case CLOX_OP_RETURN: {
                clox_print_value(clox_stack_pop());
                printf("\n");
                return CLOX_INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
}
