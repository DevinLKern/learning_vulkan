#ifndef ROSINA_SPARSE_SET_H
#define ROSINA_SPARSE_SET_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>


typedef struct SparseSet {
    uint64_t size;
    uint64_t byte_count;
    void* bytes;
} SparseSet;

#define SparseSet_Impl(type)                                                            \
    void type##_sparse_set_create(SparseSet set [static 1], const uint64_t capacity) {  \
        set->byte_count = (sizeof(type##) + sizeof(uint64_t)) * capacity;               \
        set->memory = malloc(set->byte_count);                                          \
    }
    bool sparse_set_add(SparseSet set [static 1], const uint64_t index);

void* sparse_set_remove(SparseSet set [static 1], const uint64_t index);

#endif