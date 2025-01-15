#ifndef SOMETHING_H
#define SOMETHING_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

bool LoadFile(void* const data, size_t* const data_size, const char* const file_name);

bool LoadTextFile(char* const data, size_t* const data_size, const char* const file_name);

#endif