#include <stdio.h>
#include <stdlib.h>

#include "clox/common.h"
#include "clox/compiler.h"
#include "clox/scanner.h"
#include "clox/chunk.h"
#include "clox/debug.h"

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

typedef void (*parse_fn)();

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence_type precedence;
} parse_rule;

static void grouping();
static void unary();
static void binary();
static void number();
static void error_at_current(const char *message);
static void advance();
static void error_at(clox_token *token, const char *message);
static void consume(clox_token_type type, const char *message);
static clox_chunk *current_chunk();
static void end_compiler();
static void emit_return();
static void emit_return();
static void expression();
static void emit_constant(clox_value value);
static uint8_t make_constant(clox_value value);
static void parse_precedence(precedence_type precedence);

parser_state parser;
clox_chunk *compiling_chunk;

parse_rule rules[] = {
    [CLOX_TOKEN_LEFT_PAREN]     = { grouping, NULL, PREC_NONE },
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
    [CLOX_TOKEN_BANG]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_BANG_EQUAL]     = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_EQUAL]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_EQUAL_EQUAL]    = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_GREATER]        = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_GREATER_EQUAL]  = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_LESS]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_LESS_EQUAL]     = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_IDENTIFIER]     = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_STRING]         = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_NUMBER]         = { number, NULL, PREC_NONE },
    [CLOX_TOKEN_AND]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_CLASS]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_ELSE]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_FALSE]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_FOR]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_FUN]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_IF]             = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_NIL]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_OR]             = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_PRINT]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_RETURN]         = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_SUPER]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_THIS]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_TRUE]           = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_VAR]            = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_WHILE]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_ERROR]          = { NULL, NULL, PREC_NONE },
    [CLOX_TOKEN_EOF]            = { NULL, NULL, PREC_NONE }
};

bool clox_compile(const char *source, clox_chunk *chunk)
{
    clox_init_scanner(source);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    expression();
    consume(CLOX_TOKEN_EOF, "Expected end of expression.");
    end_compiler();
    return !parser.had_error;
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

static void error_at_current(const char *message)
{
    error_at(&parser.current, message);
}

static void error(const char *message)
{
    error_at(&parser.current, message);
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

static clox_chunk *current_chunk()
{
    return compiling_chunk;
}

static void end_compiler()
{
    emit_return();

#ifdef CLOX_DEBUG_PRINT_CODE
    if (!parser.had_error) {
        clox_disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void emit_return()
{
    emit_byte(CLOX_OP_RETURN);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static void expression()
{
    parse_precedence(PREC_ASSIGMENT);
}

static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(value);
}

static void emit_constant(clox_value value)
{
    emit_bytes(CLOX_OP_CONSTANT, make_constant(value));
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

static void grouping() 
{
    expression();
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary()
{
    clox_token_type operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case CLOX_TOKEN_MINUS: emit_byte(CLOX_OP_NEGATE);
        default: return;
    }
}

static parse_rule *get_rule(clox_token_type type)
{
    return &rules[type];
}

static void parse_precedence(precedence_type precedence)
{
    advance();

    parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;

    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    prefix_rule();

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        parse_fn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

static void binary()
{
    clox_token_type operator_type = parser.previous.type;
    parse_rule *rule = get_rule(operator_type);

    parse_precedence((precedence_type)(rule->precedence + 1));

    switch (operator_type) {
        case CLOX_TOKEN_PLUS: emit_byte(CLOX_OP_ADD); break;
        case CLOX_TOKEN_MINUS: emit_byte(CLOX_OP_SUBTRACT); break;
        case CLOX_TOKEN_STAR: emit_byte(CLOX_OP_MULTIPLY); break;
        case CLOX_TOKEN_SLASH: emit_byte(CLOX_OP_DEVIDE); break;
        default: return;
    }
}

