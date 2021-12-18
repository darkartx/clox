#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clox/common.h"
#include "clox/chunk.h"
#include "clox/vm.h"
#include "clox/debug.h"

static void repl();
static void run_file(const char *path);
static char *read_file(const char *path);

int main(int argc, const char *argv[])
{
    clox_init_vm();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
        exit(64);
    }

    return 0;
}

static void repl()
{
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        clox_interpret(line);
    }
}

static void run_file(const char *path)
{
    char *source = read_file(path);
    clox_interpret_result result = clox_interpret(source);
    free(source);

    if (result == CLOX_INTERPRET_COMPILE_ERROR) exit(65);
    if (result == CLOX_INTERPRET_RUNTIME_ERROR) exit(70);
}

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough mamory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}