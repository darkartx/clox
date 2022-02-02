#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

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
static bool call_value(clox_value callee, int args_count);
static bool call(clox_obj_function* function, int arg_count);
static clox_value clock_native(int arg_count, clox_value* args);
static void define_native_function(const char* name, clox_native_fn function);

void clox_init_vm()
{
    reset_stack();
    clox_vm_instance.objects = NULL;

    clox_init_table(&clox_vm_instance.strings);
    clox_init_table(&clox_vm_instance.globals);

    define_native_function("clock", clock_native);
}

void clox_free_vm()
{
    clox_free_table(&clox_vm_instance.globals);
    clox_free_table(&clox_vm_instance.strings);
    free_objects();
}

clox_interpret_result clox_interpret(const char *source)
{
    clox_obj_function* function = clox_compile(source);
    if (function == NULL) return CLOX_INTERPRET_COMPILE_ERROR;

    clox_stack_push(CLOX_OBJ_VAL(function));
    call(function, 0);
    
    return run();
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
    clox_call_frame* frame = &clox_vm_instance.frames[clox_vm_instance.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STRING() CLOX_AS_STRING(READ_CONSTANT())
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
        clox_disassemble_instruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
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
                clox_value result = clox_stack_pop();
                clox_vm_instance.frame_count--;

                if (clox_vm_instance.frame_count == 0) {
                    clox_stack_pop();
                    return CLOX_INTERPRET_OK;
                }

                clox_vm_instance.stack_top = frame->slots;
                clox_stack_push(result);
                frame = &clox_vm_instance.frames[clox_vm_instance.frame_count - 1];
                break;
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
            case CLOX_OP_LESS: BINARY_OP(CLOX_BOOL_VAL, <); break;
            case CLOX_OP_PRINT: {
                clox_print_value(clox_stack_pop());
                printf("\n");
                break;
            }
            case CLOX_OP_POP: clox_stack_pop(); break;
            case CLOX_OP_DEFINE_GLOBAL: {
                clox_obj_string* name = READ_STRING();
                clox_table_set(&clox_vm_instance.globals, name, clox_stack_peek(0));
                clox_stack_pop();
                break;
            }
            case CLOX_OP_GET_GLOBAL: {
                clox_obj_string* name = READ_STRING();
                clox_value value;
                if (!clox_table_get(&clox_vm_instance.globals, name, &value)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return CLOX_INTERPRET_RUNTIME_ERROR;
                }
                clox_stack_push(value);
                break;
            }
            case CLOX_OP_SET_GLOBAL: {
                clox_obj_string* name = READ_STRING();
                if (clox_table_set(&clox_vm_instance.globals, name, clox_stack_peek(0))) {
                    clox_table_delete(&clox_vm_instance.globals, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return CLOX_INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case CLOX_OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                clox_stack_push(frame->slots[slot]);
                break;
            }
            case CLOX_OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = clox_stack_peek(0);
                break;
            }
            case CLOX_OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (is_falsey(clox_stack_peek(0))) frame->ip += offset;
                break;
            }
            case CLOX_OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case CLOX_OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case CLOX_OP_CALL: {
                int arg_count = READ_BYTE();
                if (!call_value(clox_stack_peek(arg_count), arg_count)) {
                    return CLOX_INTERPRET_RUNTIME_ERROR;
                }
                frame = &clox_vm_instance.frames[clox_vm_instance.frame_count - 1];
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

static void reset_stack()
{
    clox_vm_instance.stack_top = clox_vm_instance.stack;
    clox_vm_instance.frame_count = 0;
}

static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = clox_vm_instance.frame_count - 1; i >= 0; i--) {
        clox_call_frame* frame = &clox_vm_instance.frames[i];
        clox_obj_function* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    reset_stack();
}

static void define_native_function(const char* name, clox_native_fn function)
{
    clox_stack_push(CLOX_OBJ_VAL(clox_copy_string(name, (int)strlen(name))));
    clox_stack_push(CLOX_OBJ_VAL(clox_new_native_function(function)));
    clox_table_set(&clox_vm_instance.globals, CLOX_AS_STRING(clox_vm_instance.stack[0]), clox_vm_instance.stack[1]);
    clox_stack_pop();
    clox_stack_pop();
}

static bool is_falsey(clox_value value)
{
    return CLOX_IS_NIL(value) || (CLOX_IS_BOOL(value) && !CLOX_AS_BOOL(value));
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

static bool call(clox_obj_function* function, int arg_count)
{
    if (arg_count != function->arity) {
        runtime_error("Expect %d arguments but got %d.", function->arity, arg_count);
        return false;
    }

    if (clox_vm_instance.frame_count == CLOX_FRAME_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    clox_call_frame* frame = &clox_vm_instance.frames[clox_vm_instance.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = clox_vm_instance.stack_top - arg_count - 1;
    return true;
}

static bool call_value(clox_value callee, int args_count)
{
    if (CLOX_IS_OBJ(callee)) {
        switch (CLOX_OBJ_TYPE(callee)) {
            case CLOX_OBJ_FUNCTION:
                return call(CLOX_AS_FUNCTION(callee), args_count);
            case CLOX_OBJ_NATIVE_FUNCTION: {
                clox_native_fn native = CLOX_AS_NATIVE_FUNCTION(callee);
                clox_value result = native(args_count, clox_vm_instance.stack_top - args_count);
                clox_vm_instance.stack_top -= args_count + 1;
                clox_stack_push(result);
                return true;
            }
            default:
                break;
        }
    }

    runtime_error("Can only call functions and classes.");
    return false;
}

static clox_value clock_native(int arg_count, clox_value* args)
{
    return CLOX_NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
