#ifndef ROSINA_LOG_H
#define ROSINA_LOG_H

#include <stdio.h>


#define ROSINA_LOG_ERROR(...) \
    fprintf(stdout, "[ERROR] FILE: \"%s\", LINE: %d, MESSAGE: \"", __FILE__, __LINE__); \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\"\n")

#define ROSINA_LOG_INFO(...) \
    fprintf(stdout, "[INFO] FILE: \"%s\", LINE: %d, MESSAGE: \"", __FILE__, __LINE__); \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\"\n")

#endif