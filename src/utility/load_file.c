#include <utility/load_file.h>

#include <stdio.h>

bool LoadFile(void* const data, size_t* const data_size, const char* const file_name) {
    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) return true;

    //  Go to the end of the file.
    if (fseek(fp, 0L, SEEK_END) != 0) return true;

    // get the poisition of the file pointer
    *data_size = ftell(fp);

    if (fseek(fp, 0L, SEEK_SET) != 0) return true;

    if (data == NULL) {
        fclose(fp);
        return false;
    }

    if (fread(data, 1, (*data_size), fp) != (*data_size)) {
        fclose(fp);
        return true;
    }

    fclose(fp);
    return false;
}

bool LoadTextFile(char* const data, size_t* const data_size, const char* const file_name) {
    FILE *fp = fopen(file_name, "r");
    if (fp == NULL) return true;

    //  Go to the end of the file.
    if (fseek(fp, 0L, SEEK_END) != 0) return true;

    // get the poisition of the file pointer
    *data_size = ftell(fp);

    if (fseek(fp, 0L, SEEK_SET) != 0) return true;

    if (data == NULL) {
        fclose(fp);
        return false;
    }

    if (fread(data, 1, (*data_size), fp) != (*data_size)) {
        fclose(fp);
        return true;
    }

    fclose(fp);
    return false;
}