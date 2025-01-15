#include <utility/memory_arena.h>

#include <stdlib.h>
#include <string.h>

MemoryArena MemoryArena_Create(const uint64_t capacity) {
    void* const bytes = malloc(capacity);
    memset(bytes, 0, capacity);

    MemoryArena arena = {
        .memory = bytes,
        .alloc_pos = bytes
    };
    return arena;
}

void* MemoryArena_Allocate(MemoryArena arena [static 1], const uint64_t size) {
    void* pointer = arena->alloc_pos;
    arena->alloc_pos += size;
    return pointer;
}

void MemoryArena_Free(MemoryArena arena [static 1]) {
    free(arena->memory);
    arena->memory = NULL;
    arena->alloc_pos = NULL;
}