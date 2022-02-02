#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clox/common.h"
#include "clox/compiler.h"
#include "clox/scanner.h"
#include "clox/chunk.h"
#include "clox/debug.h"
#include "clox/object.h"

typedef struct {
    clox_token current;
    clox_token previous;
    bool had_error;
    bool panic_mode;
} parser_state;

typedef enum {
    PREC_NONE,
    PREC_ASSIGMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARSION,
    PREC_TERM,
    PREC_FRACOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} precedence_type;

typedef void (*parse_fn)(bool can_assign);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence_type precedence;
} parse_rule;

typedef struct {
    clox_token name;
    int depth;
} local;

typedef enum {
    FUNCTION_TYPE_FUNCTION,
    FUNCTION_TYPE_SCRIPT
} function_type;

typedef struct compiler {
    struct compiler* enclosing;
    local locals[CLOX_UINT8_COUNT];
    int local_count;
    int scope_depth;
    clox_obj_function* function;
    function_type function_type;
} compiler;

static void init_compiler(compiler* compiler, function_type type);
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void number(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void variable(bool can_assign);
static void and_(bool can_assign);
static void or_(bool can_assign);
static void call(bool can_assign);
static void advance();
static bool match(clox_token_type type);
static void declaration();
static clox_obj_function* end_compiler();
static void statement();

parser_state parser;
compiler* current = NULL;

parse_rule rules[] = {
    [CLOX_TOKEN_LEFT_PAREN]     = { grouping, call, PREC_NONE },
    [CLOX_TOKEN_RIGHT_PAREN]    = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_LEFT_BRACE]     = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_RIGHT_BRACE]    = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_COMMA]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_DOT]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_MINUS]          = { unary, binary, PREC_TERM },
    [CLOX_TOKEN_PLUS]           = { NULL, binary, PREC_TERM },
    [CLOX_TOKEN_SEMICOLON]      = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_SLASH]          = { NULL, binary, PREC_FRACOR },
    [CLOX_TOKEN_STAR]           = { NULL, binary, PREC_FRACOR },
    [CLOX_TOKEN_BANG]           = { unary, NULL, PREC_NONE },
    [CLOX_TOKEN_BANG_EQUAL]     = { NULL, binary, PREC_EQUALITY },
    [CLOX_TOKEN_EQUAL]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_EQUAL_EQUAL]    = { NULL, binary, PREC_EQUALITY },
    [CLOX_TOKEN_GREATER]        = { NULL, binary, PREC_COMPARSION },
    [CLOX_TOKEN_GREATER_EQUAL]  = { NULL, binary, PREC_COMPARSION },
    [CLOX_TOKEN_LESS]           = { NULL, binary, PREC_COMPARSION },
    [CLOX_TOKEN_LESS_EQUAL]     = { NULL, binary, PREC_COMPARSION },
    [CLOX_TOKEN_IDENTIFIER]     = { variable, NULL, PREC_NONE },
    [CLOX_TOKEN_STRING]         = { string, NULL, PREC_NONE },
    [CLOX_TOKEN_NUMBER]         = { number, NULL, PREC_NONE },
    [CLOX_TOKEN_AND]            = { NULL, and_, PREC_AND },
    [CLOX_TOKEN_CLASS]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_ELSE]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_FALSE]          = { literal, NULL, PREC_NONE },
    [CLOX_TOKEN_FOR]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_FUN]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_IF]             = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_NIL]            = { literal, NULL, PREC_NONE },
    [CLOX_TOKEN_OR]             = { NULL, or_, PREC_OR },
    [CLOX_TOKEN_PRINT]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_RETURN]         = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_SUPER]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_THIS]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_TRUE]           = { literal, NULL, PREC_NONE },
    [CLOX_TOKEN_VAR]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_WHILE]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_ERROR]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_EOF]            = { NULL, NULL, PREC_NONE }
};

clox_obj_function* clox_compile(const char *source)
{
    clox_init_scanner(source);
    compiler compiler;
    init_compiler(&compiler, FUNCTION_TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    
    while (!match(CLOX_TOKEN_EOF)) {
        declaration();
    }

    clox_obj_function* function = end_compiler();
    return parser.had_error ? NULL : function;
}

static void init_compiler(compiler* compiler, function_type type)
{
    compiler->enclosing = current;
    compiler->function_type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function = clox_new_function();
    current = compiler;

    if (type != FUNCTION_TYPE_SCRIPT) {
        current->function->name = clox_copy_string(parser.previous.start, parser.previous.length);
    }

    local* local = &current->locals[current->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static bool check(clox_token_type type) 
{
    return parser.current.type == type;
}

static void error_at(clox_token *token, const char *message)
{
    if (parser.panic_mode) return;
    parser.panic_mode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == CLOX_TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == CLOX_TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static clox_chunk* current_chunk()
{
    return &current->function->chunk;
}

static parse_rule *get_rule(clox_token_type type)
{
    return &rules[type];
}

static void mark_initialized()
{
    if (current->scope_depth == 0) return;
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void error_at_current(const char *message)
{
    error_at(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = clox_scan_token();
        if (parser.current.type != CLOX_TOKEN_ERROR) break;

        error_at_current(parser.current.start);
    }
}

static void error(const char *message)
{
    error_at(&parser.current, message);
}

static void consume(clox_token_type type, const char *message) 
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

static void emit_byte(uint8_t byte)
{
    clox_write_chunk(current_chunk(), byte, parser.current.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_return()
{
    emit_byte(CLOX_OP_NIL);
    emit_byte(CLOX_OP_RETURN);
}

static clox_obj_function* end_compiler()
{
    emit_return();
    clox_obj_function* function = current->function;

#ifdef CLOX_DEBUG_PRINT_CODE
    if (!parser.had_error) {
        clox_disassemble_chunk(
            current_chunk(), 
            function->name != NULL ? function->name->chars : "<script>"
        );
    }
#endif

    current = current->enclosing;
    return function;
}

static void parse_precedence(precedence_type precedence)
{
    advance();

    parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;

    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGMENT;
    prefix_rule(can_assign);

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        parse_fn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && match(CLOX_TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static void expression()
{
    parse_precedence(PREC_ASSIGMENT);
}

static uint8_t make_constant(clox_value value)
{
    int constant = clox_chunk_add_constant(current_chunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emit_constant(clox_value value)
{
    emit_bytes(CLOX_OP_CONSTANT, make_constant(value));
}

static void number(bool can_assign)
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(CLOX_NUMBER_VAL(value));
}

static void grouping(bool can_assign) 
{
    expression();
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool can_assign)
{
    clox_token_type operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
    case CLOX_TOKEN_BANG: emit_byte(CLOX_OP_NOT); break;
    case CLOX_TOKEN_MINUS: emit_byte(CLOX_OP_NEGATE); break;
    default: return;
    }
}

static void binary(bool can_assign)
{
    clox_token_type operator_type = parser.previous.type;
    parse_rule *rule = get_rule(operator_type);

    parse_precedence((precedence_type)(rule->precedence + 1));

    switch (operator_type) {
        case CLOX_TOKEN_BANG_EQUAL: emit_bytes(CLOX_OP_EQUAL, CLOX_OP_NOT); break;
        case CLOX_TOKEN_EQUAL_EQUAL: emit_byte(CLOX_OP_EQUAL); break;
        case CLOX_TOKEN_GREATER: emit_byte(CLOX_OP_GREATER); break;
        case CLOX_TOKEN_GREATER_EQUAL: emit_bytes(CLOX_OP_LESS, CLOX_OP_NOT); break;
        case CLOX_TOKEN_LESS: emit_byte(CLOX_OP_LESS); break;
        case CLOX_TOKEN_LESS_EQUAL: emit_bytes(CLOX_OP_GREATER, CLOX_OP_NOT); break;
        case CLOX_TOKEN_PLUS: emit_byte(CLOX_OP_ADD); break;
        case CLOX_TOKEN_MINUS: emit_byte(CLOX_OP_SUBTRACT); break;
        case CLOX_TOKEN_STAR: emit_byte(CLOX_OP_MULTIPLY); break;
        case CLOX_TOKEN_SLASH: emit_byte(CLOX_OP_DEVIDE); break;
        default: return;
    }
}

static void literal(bool can_assign)
{
    switch (parser.previous.type) {
        case CLOX_TOKEN_FALSE: emit_byte(CLOX_OP_FALSE); break;
        case CLOX_TOKEN_NIL: emit_byte(CLOX_OP_NIL); break;
        case CLOX_TOKEN_TRUE: emit_byte(CLOX_OP_TRUE); break;
        default: return;
    }
}

static void string(bool can_assign)
{
    emit_constant(CLOX_OBJ_VAL(clox_copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void print_statement()
{
    expression();
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(CLOX_OP_PRINT);
}

static void emit_loop(int loop_start)
{
    emit_byte(CLOX_OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    current->scope_depth--;

    while (current->local_count > 0 && current->locals[current->local_count - 1].depth > current->scope_depth) {
        emit_byte(CLOX_OP_POP);
        current->local_count--;
    }
}

static void patch_jump(int offset)
{
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too match code to jump over.");
    }

    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static void expression_statement()
{
    expression();
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(CLOX_OP_POP);
}

static uint8_t identifier_constant(clox_token* name)
{
    return make_constant(
        CLOX_OBJ_VAL(clox_copy_string(name->start, name->length))
    );
}

static void define_variable(uint8_t global)
{
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }

    emit_bytes(CLOX_OP_DEFINE_GLOBAL, global);
}

static void add_local(clox_token name)
{
    if (current->local_count == CLOX_UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    local* local = &current->locals[current->local_count++];
    local->name = name;
    local->depth = -1;
}

static bool identifiers_equal(clox_token* a, clox_token* b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declare_variable()
{
    if (current->scope_depth == 0) return;

    clox_token* name = &parser.previous;

    for (int i = current->local_count - 1; i >= 0; i--) {
        local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth) {
            break;
        }

        if (identifiers_equal(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    add_local(*name);
}

static uint8_t parse_variable(const char* error_message)
{
    consume(CLOX_TOKEN_IDENTIFIER, error_message);

    declare_variable();
    if (current->scope_depth > 0) return 0;

    return identifier_constant(&parser.previous);
}

static void var_declaration()
{
    uint8_t global = parse_variable("Expect variable name.");

    if (match(CLOX_TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(CLOX_OP_NIL);
    }

    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    define_variable(global);
}

static void for_statement()
{
    begin_scope();

    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(CLOX_TOKEN_SEMICOLON)) {
    } else if (match(CLOX_TOKEN_VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }

    int loop_start = current_chunk()->count;
    int exit_jump = -1;
    if (!match(CLOX_TOKEN_SEMICOLON)) {
        expression();
        consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exit_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
        emit_byte(CLOX_OP_POP);
    }

    if (!match(CLOX_TOKEN_RIGHT_PAREN)) {
        int body_jump = emit_jump(CLOX_OP_JUMP);
        int increment_start = current_chunk()->count;

        expression();
        emit_byte(CLOX_OP_POP);
        consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(loop_start);

    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_byte(CLOX_OP_POP);
    }

    end_scope();
}

static int emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);

    return current_chunk()->count - 2;
}

static void if_statement()
{
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int then_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    emit_byte(CLOX_OP_POP);
    statement();

    int else_jump = emit_jump(CLOX_OP_JUMP);

    patch_jump(then_jump);
    emit_byte(CLOX_OP_POP);

    if (match(CLOX_TOKEN_ELSE)) statement();

    patch_jump(else_jump);
}

static void while_statement()
{
    int loop_start = current_chunk()->count;
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    emit_byte(CLOX_OP_POP);

    statement();
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_byte(CLOX_OP_POP);
}

static void block()
{
    while (!check(CLOX_TOKEN_RIGHT_BRACE) && !check(CLOX_TOKEN_EOF)) {
        declaration();
    }

    consume(CLOX_TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void return_statement()
{
    if (current->function_type == FUNCTION_TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(CLOX_TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        expression();
        consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after return value.");
        emit_byte(CLOX_OP_RETURN);
    }
}

static void statement()
{
    if (match(CLOX_TOKEN_PRINT)) {
        print_statement();
    } else if (match(CLOX_TOKEN_FOR)) {
        for_statement();
    } else if (match(CLOX_TOKEN_IF)) {
        if_statement();
    } else if (match(CLOX_TOKEN_RETURN)) {
        return_statement();
    } else if (match(CLOX_TOKEN_WHILE)) {
        while_statement();
    } else if (match(CLOX_TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

static void synchronize()
{
    parser.panic_mode = false;

    while (parser.current.type != CLOX_TOKEN_EOF) {
        if (parser.previous.type == CLOX_TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case CLOX_TOKEN_CLASS:
            case CLOX_TOKEN_FUN:
            case CLOX_TOKEN_VAR:
            case CLOX_TOKEN_FOR:
            case CLOX_TOKEN_IF:
            case CLOX_TOKEN_WHILE:
            case CLOX_TOKEN_PRINT:
            case CLOX_TOKEN_RETURN:
                return;
            default:
                ;
        }

        advance();
    }
}

static void function(function_type type)
{
    compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();

    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    if (!check(CLOX_TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error_at_current("Can't have more than 255 parameters.");
            }
            uint8_t constant = parse_variable("Expect parameter name.");
            define_variable(constant);
        } while (match(CLOX_TOKEN_COMMA));
    }

    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(CLOX_TOKEN_LEFT_BRACE, "Expect '{' before function body.");


    block();

    clox_obj_function* function = end_compiler();
    emit_bytes(CLOX_OP_CONSTANT, make_constant(CLOX_OBJ_VAL(function)));
}

static void fun_declaration()
{
    uint8_t global = parse_variable("Expect function name.");
    mark_initialized();
    function(FUNCTION_TYPE_FUNCTION);
    define_variable(global);
}

static void declaration()
{
    if (match(CLOX_TOKEN_FUN)) {
        fun_declaration();
    } else if (match(CLOX_TOKEN_VAR)) {
        var_declaration();
    } else {
        statement();
    }

    if (parser.panic_mode) synchronize();
}

static bool match(clox_token_type type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

static int resolve_local(compiler* compiler, clox_token* name)
{
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void named_variable(clox_token name, bool can_assign)
{
    uint8_t get_op, set_op;
    uint8_t arg = resolve_local(current, &name);

    if (arg != -1) {
        get_op = CLOX_OP_GET_LOCAL;
        set_op = CLOX_OP_SET_LOCAL;
    } else {
        arg = identifier_constant(&name);
        get_op = CLOX_OP_GET_GLOBAL;
        set_op = CLOX_OP_SET_GLOBAL;
    }

    if (can_assign && match(CLOX_TOKEN_EQUAL)) {
        expression();
        emit_bytes(set_op, arg);
    } else {
        emit_bytes(get_op, arg);
    }
}

static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

static void and_(bool can_assign)
{
    int end_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);

    emit_byte(CLOX_OP_POP);
    parse_precedence(PREC_AND);

    patch_jump(end_jump);
}

static void or_(bool can_assign)
{
    int else_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(CLOX_OP_JUMP);

    patch_jump(else_jump);
    emit_byte(CLOX_OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static uint8_t argument_list()
{
    uint8_t arg_count = 0;
    if (!check(CLOX_TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (arg_count == 255) {
                error("Can't have more than 255 arguments.");
            }
            arg_count++;
        } while (match(CLOX_TOKEN_COMMA));
    }

    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void call(bool can_assign)
{
    uint8_t arg_count = argument_list();
    emit_bytes(CLOX_OP_CALL, arg_count);
}



