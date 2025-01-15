#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include <inttypes.h>

typedef struct MemoryArena {
    void* memory; 
    void* alloc_pos;
} MemoryArena;

MemoryArena MemoryArena_Create(const uint64_t capacity);

void* MemoryArena_Allocate(MemoryArena arena [static 1], const uint64_t size);

void MemoryArena_Free(MemoryArena arena [static 1]);

#endif