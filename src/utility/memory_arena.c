#include <stdlib.h>
#include <string.h>
#include <utility/memory_arena.h>
#include <vulkan/vulkan_core.h>

MemoryArena MemoryArena_Create(const uint64_t capacity) {
    MemoryArena arena = {
        .memory = calloc(capacity, 1),
        .alloc_pos = NULL
    };
    arena.alloc_pos = arena.memory;

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