#include <stdio.h>

#include "clox/common.h"
#include "clox/compiler.h"
#include "clox/scanner.h"

void clox_compile(const char *source)
{
    clox_init_scanner(source);
    int line = 0;

    for (;;) {
        clox_token token = clox_scan_token();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }

        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == CLOX_TOKEN_EOF) break;
    }
}
