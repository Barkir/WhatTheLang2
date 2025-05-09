#include <stdlib.h>
#include "what_lang/errors.h"

int NASM2ELF(const char * filename)
{
    FILE * fp = fopen(filename, "r+");
    if (!fp) return WHAT_FILEOPEN_ERROR;

    HEADER
}
