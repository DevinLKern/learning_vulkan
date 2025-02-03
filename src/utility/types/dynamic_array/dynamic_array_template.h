#ifndef ROSINA_B_TREE_H
#define ROSINA_B_TREE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define TEMPLATE_DynamicArray(T)                                                        \
typedef struct T##DynamicArray {                                                        \
    uint64_t    size;                                                                   \
    uint64_t    capacity;                                                               \
    T*          elements;                                                               \
} T##DynamicArray;                                                                      \
static inline T##DynamicArray T##DynamicArray_Create() {                                \
    return  (T##DynamicArray){                                                          \
        .size = 0,                                                                      \
        .capacity = 16,                                                                 \
        .elements = malloc(sizeof(T) * 16)                                              \
    };                                                                                  \
}                                                                                       \
static inline void T##DynamicArray_Grow(T##DynamicArray array [static 1]) {             \
    array->capacity *= 2;                                                               \
    T* const new_elements = malloc(sizeof(T) * array->capacity);                        \
    memcpy(new_elements, array->elements, sizeof(T) * array->size);                     \
    free(array->elements);                                                              \
    array->elements = new_elements;                                                     \
}                                                                                       \
T* T##DynamicArray_PushBack(T##DynamicArray array [static 1], const T value) {          \
    if (array->size == array->capacity) T##DynamicArray_Grow(array);                    \
    array->elements[array->size] = value;                                               \
    array->size++;                                                                      \
}                                                                                       \
static inline void T##DynamicArray_Shrink(T##DynamicArray array [static 1]) {           \
    if (array->capacity == 16) return;                                                  \
    array->capacity /= 2;                                                               \
    T* const new_elements = malloc(sizeof(T) * array->capacity);                        \
    memcpy(new_elements, array->elements, sizeof(T) * array->size);                     \
    free(array->elements);                                                              \
    array->elements = new_elements;                                                     \
}                                                                                       \
T T##DynamicArray_PopBack(T##DynamicArray array [static 1]) {                           \
    if (array->size <= (array->capacity / 2)) T##DynamicArray_Shrink(array);            \
    array->size--;                                                                      \
    return array->elements[array->size];                                                \
}

#endif