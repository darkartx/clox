#include <stdio.h>
#include <string.h>

#include "clox/common.h"
#include "clox/scanner.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} _scanner;

_scanner scanner;

static bool is_at_end();
static clox_token make_token(clox_token_type type);
static clox_token error_token(const char *message);
static char advance();
static bool match(char expected);
static void skip_whitespace();
static char peek();
static char peek_next();
static clox_token string();
static bool is_digit(char c);
static clox_token number();
static bool is_alpha(char c);
static clox_token identifier();
static clox_token_type identifier_type();
static clox_token_type check_keyword(int start, int length, const char *rest, clox_token_type type);

void clox_init_scanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

clox_token clox_scan_token()
{
    skip_whitespace();

    scanner.start = scanner.current;

    if (is_at_end()) return make_token(CLOX_TOKEN_EOF);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
        case '(': return make_token(CLOX_TOKEN_LEFT_PAREN);
        case ')': return make_token(CLOX_TOKEN_RIGHT_PAREN);
        case '{': return make_token(CLOX_TOKEN_LEFT_BRACE);
        case '}': return make_token(CLOX_TOKEN_RIGHT_BRACE);
        case ';': return make_token(CLOX_TOKEN_SEMICOLON);
        case ',': return make_token(CLOX_TOKEN_COMMA);
        case '.': return make_token(CLOX_TOKEN_DOT);
        case '-': return make_token(CLOX_TOKEN_MINUS);
        case '+': return make_token(CLOX_TOKEN_PLUS);
        case '/': return make_token(CLOX_TOKEN_SLASH);
        case '*': return make_token(CLOX_TOKEN_STAR);
        case '!':
            return make_token(match('=') ? CLOX_TOKEN_BANG_EQUAL : CLOX_TOKEN_BANG);
        case '=':
            return make_token(match('=') ? CLOX_TOKEN_EQUAL_EQUAL : CLOX_TOKEN_EQUAL);
        case '<':
            return make_token(match('=') ? CLOX_TOKEN_LESS_EQUAL : CLOX_TOKEN_LESS);
        case '>':
            return make_token(match('=') ? CLOX_TOKEN_GREATER_EQUAL : CLOX_TOKEN_GREATER);
        case '"': return string();
    }

    return error_token("Unexpected character.");
}

static bool is_at_end()
{
    return *scanner.current == '\0';
}

static clox_token make_token(clox_token_type type)
{
    clox_token result;
    result.type = type;
    result.start = scanner.start;
    result.length = (int)(scanner.current - scanner.start);
    result.line = scanner.line;
    return result;
}

static clox_token error_token(const char *message)
{
    clox_token result;
    result.type = CLOX_TOKEN_ERROR;
    result.start = message;
    result.length = (int)strlen(message);
    result.line = scanner.line;
    return result;
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static bool match(char expected)
{
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static void skip_whitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peek_next()) {
                    while (peek() != '\n' && !is_at_end()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static char peek()
{
    return *scanner.current;
}

static char peek_next()
{
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

static clox_token string()
{
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    advance();
    return make_token(CLOX_TOKEN_STRING);
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static clox_token number()
{
    while (is_digit(peek())) advance();

    if (peek() == '.' && is_digit(peek_next())) {
        advance();
        while (is_digit(peek())) advance();
    }

    return make_token(CLOX_TOKEN_NUMBER);
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static clox_token identifier()
{
    while (is_alpha(peek()) || is_digit(peek())) advance();
    return make_token(identifier_type());
}

static clox_token_type identifier_type()
{
    switch (scanner.start[0]) {
        case 'a': return check_keyword(1, 2, "nd", CLOX_TOKEN_AND);
        case 'c': return check_keyword(1, 4, "lass", CLOX_TOKEN_CLASS);
        case 'e': return check_keyword(1, 3, "lse", CLOX_TOKEN_ELSE);
        case 'i': return check_keyword(1, 1, "f", CLOX_TOKEN_IF);
        case 'n': return check_keyword(1, 2, "il", CLOX_TOKEN_NIL);
        case 'o': return check_keyword(1, 1, "r", CLOX_TOKEN_OR);
        case 'p': return check_keyword(1, 4, "rint", CLOX_TOKEN_PRINT);
        case 'r': return check_keyword(1, 5, "eturn", CLOX_TOKEN_RETURN);
        case 's': return check_keyword(1, 4, "uper", CLOX_TOKEN_SUPER);
        case 'v': return check_keyword(1, 2, "ar", CLOX_TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile", CLOX_TOKEN_WHILE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", CLOX_TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r", CLOX_TOKEN_FOR);
                    case 'u': return check_keyword(2, 1, "n", CLOX_TOKEN_FUN);
                }
            }
            break;
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'r': return check_keyword(2, 2, "ue", CLOX_TOKEN_TRUE);
                    case 'h': return check_keyword(2, 2, "is", CLOX_TOKEN_THIS);
                }
            }
            break;
    }

    return CLOX_TOKEN_IDENTIFIER;
}

static clox_token_type check_keyword(int start, int length, const char *rest, clox_token_type type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return CLOX_TOKEN_IDENTIFIER;
}
