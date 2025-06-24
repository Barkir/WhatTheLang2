#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "what_lang/constants.h"
#include "what_lang/IO.h"
#include "what_lang/hashtable_errors.h"


int ProcessCmd(int argc, char * argv[])
{
    if (argc >= 2)
    {
        if (!strcmp(argv[1], "--default"))  return IO_DEFAULT;
        else if (!strcmp(argv[1], "--simd")) return IO_ALIGNED;
    }

    return -fprintf(stderr, "Typo in: ./run <--default / --simd>\n");
}

int File2Lines(int a_flag, char *** buf, const char * filename)
{

    FILE * fp = fopen(filename, "r+");
    if (!fp)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }

    struct stat stats;
    if (fstat(fileno(fp), &stats) == -1)
    {
        perror("fstat");
        fclose(fp);
        return EXIT_FAILURE;
    }
    size_t fsize = stats.st_size;

    if (a_flag == IO_ALIGNED)
    {
        *buf = (char**) aligned_alloc(BUF_LEN, 2 * fsize + (2 * fsize) % BUF_LEN);
        memset(*buf, 0, 2 * fsize + (2 * fsize) % BUF_LEN);
    }
    else if (a_flag == IO_DEFAULT)      *buf = (char**) calloc(2 * fsize + 1, sizeof(char));

    if (!buf)
    {
        perror("book allocation");
        fclose(fp);
        return EXIT_FAILURE;
    }

    // if (setvbuf(fp, **buf, _IOFBF, ALIGNED_SIZE))
    // {
    //     perror("setvbuf");
    //     free(*buf);
    //     fclose(fp);
    //     return EXIT_FAILURE;
    // }

    char line[BUF_LEN + 1];
    int count = 0;
    while (fgets(line, BUF_LEN + 1, fp) && count < NUM_WORDS * 300)
    {
        if (a_flag == IO_ALIGNED)
        {
            // if (strlen(line) == BUF_LEN)
            {
                *((*buf) + count) = (char*) aligned_alloc(BUF_LEN, BUF_LEN);
                memset(*(*buf) + count, 0, BUF_LEN);
                memcpy(*(*buf) + count, line, strlen(line) + 1);
                count++;
            }
        }
        else
        {
            *((*buf) + count) = (char*) calloc(BUF_LEN, 1);
            memcpy(*((*buf) + count), line, strlen(line) + 1);
            // strdup(line);
            count++;
        }

    }

    fclose(fp);
    return 0;
}
