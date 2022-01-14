#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "clox/common.h"
#include "clox/vm.h"
#include "clox/debug.h"
#include "clox/compiler.h"
#include "clox/value.h"
#include "clox/object.h"
#include "memory.h"

clox_vm clox_vm_instance;

static void reset_stack();
static clox_interpret_result run();
static void runtime_error(const char *format, ...);
static bool is_falsey(clox_value value);
static void concatenate();

void clox_init_vm()
{
    reset_stack();
    clox_vm_instance.objects = NULL;

    clox_init_table(&clox_vm_instance.strings);
}

void clox_free_vm()
{
    clox_free_table(&clox_vm_instance.strings);
    free_objects();
}

clox_interpret_result clox_interpret(const char *source)
{
    clox_chunk chunk;
    clox_init_chunk(&chunk);

    if (!clox_compile(source, &chunk)) {
        clox_free_chunk(&chunk);
        return CLOX_INTERPRET_COMPILE_ERROR;
    }

    clox_vm_instance.chunk = &chunk;
    clox_vm_instance.ip = clox_vm_instance.chunk->code;

    clox_interpret_result result = run();

    clox_free_chunk(&chunk);
    return result;
}

void clox_stack_push(clox_value value)
{
    *clox_vm_instance.stack_top = value;
    clox_vm_instance.stack_top++;
}

clox_value clox_stack_pop()
{
    clox_vm_instance.stack_top--;
    return *clox_vm_instance.stack_top;
}

clox_value clox_stack_peek(int distance)
{
    return clox_vm_instance.stack_top[-1 - distance];
}

static clox_interpret_result run()
{
#define READ_BYTE() (*clox_vm_instance.ip++)
#define READ_CONSTANT() (clox_vm_instance.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op) \
    do { \
        if ((!CLOX_IS_NUMBER(clox_stack_peek(0)) || (!CLOX_IS_NUMBER(clox_stack_peek(1))))) { \
            runtime_error("Operands must be numbers."); \
            return CLOX_INTERPRET_RUNTIME_ERROR; \
        } \
        double b = CLOX_AS_NUMBER(clox_stack_pop()); \
        double a = CLOX_AS_NUMBER(clox_stack_pop()); \
        clox_stack_push(value_type(a op b)); \
    } while (false)

    for (;;) {
#ifdef CLOX_DEBUG_TRACE_EXECUTION
        printf("      ");
        for (clox_value *slot = clox_vm_instance.stack; slot < clox_vm_instance.stack_top; slot++) {
            printf("[ ");
            clox_print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        clox_disassemble_instruction(clox_vm_instance.chunk, (int)(clox_vm_instance.ip - clox_vm_instance.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case CLOX_OP_CONSTANT: {
                clox_value constant = READ_CONSTANT();
                clox_stack_push(constant);
                break;
            }
            case CLOX_OP_ADD: {
                if (CLOX_IS_STRING(clox_stack_peek(0)) && CLOX_IS_STRING(clox_stack_peek(1))) {
                    concatenate();
                } else if (CLOX_IS_NUMBER(clox_stack_peek(0)) && CLOX_IS_NUMBER(clox_stack_peek(1))) {
                    double b = CLOX_AS_NUMBER(clox_stack_pop());
                    double a = CLOX_AS_NUMBER(clox_stack_pop());
                    clox_stack_push(CLOX_NUMBER_VAL(a + b));
                } else {
                    runtime_error("Operands must be two numbers or two strings.");
                    return CLOX_INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case CLOX_OP_SUBTRACT: {
                BINARY_OP(CLOX_NUMBER_VAL, -);
                break;
            }
            case CLOX_OP_MULTIPLY: {
                BINARY_OP(CLOX_NUMBER_VAL, *);
                break;
            }
            case CLOX_OP_DEVIDE: {
                BINARY_OP(CLOX_NUMBER_VAL, /);
                break;
            }
            case CLOX_OP_NEGATE: {
                if (!CLOX_IS_NUMBER(clox_stack_peek(0))) {
                    runtime_error("Operand must be number.");
                    return CLOX_INTERPRET_RUNTIME_ERROR;
                }

                clox_stack_push(
                    CLOX_NUMBER_VAL(-CLOX_AS_NUMBER(clox_stack_pop()))
                );
                break;
            }
            case CLOX_OP_RETURN: {
                clox_print_value(clox_stack_pop());
                printf("\n");
                return CLOX_INTERPRET_OK;
            }
            case CLOX_OP_NIL: clox_stack_push(CLOX_NIL_VAL); break;
            case CLOX_OP_TRUE: clox_stack_push(CLOX_BOOL_VAL(true)); break;
            case CLOX_OP_FALSE: clox_stack_push(CLOX_BOOL_VAL(false)); break;
            case CLOX_OP_NOT: 
                clox_stack_push(CLOX_BOOL_VAL(!is_falsey(clox_stack_pop())));
                break;
            case CLOX_OP_EQUAL:
                clox_value b = clox_stack_pop();
                clox_value a = clox_stack_pop();
                clox_stack_push(CLOX_BOOL_VAL(clox_value_equal(a, b)));
                break;
            case CLOX_OP_GREATER: BINARY_OP(CLOX_BOOL_VAL, >); break;
            case CLOX_OP_LESS: BINARY_OP(CLOX_BOOL_VAL, >); break;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

static void reset_stack()
{
    clox_vm_instance.stack_top = clox_vm_instance.stack;
}

static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = clox_vm_instance.ip - clox_vm_instance.chunk->code - 1;
    int line = clox_vm_instance.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

static bool is_falsey(clox_value value)
{
    return CLOX_IS_NIL(value) || (CLOX_IS_BOOL(value) && CLOX_AS_BOOL(value));
}

static void concatenate()
{
    clox_obj_string *b = CLOX_AS_STRING(clox_stack_pop());
    clox_obj_string *a = CLOX_AS_STRING(clox_stack_pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    clox_obj_string* result = clox_take_string(chars, length);
    clox_stack_push(CLOX_OBJ_VAL(result));
}
