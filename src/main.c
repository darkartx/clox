#include "clox/common.h"
#include "clox/chunk.h"
#include "clox/debug.h"

int main(int argc, const char *argv[])
{
    clox_chunk chunk;
    clox_init_chunk(&chunk);

    int constant = clox_chunk_add_constant(&chunk, 1.2);
    clox_write_chunk(&chunk, CLOX_OP_CONSTANT, 123);
    clox_write_chunk(&chunk, constant, 123);

    clox_write_chunk(&chunk, CLOX_OP_RETURN, 123);

    clox_disassemble_chunk(&chunk, "test chunk");

    clox_free_chunk(&chunk);
    return 0;
}