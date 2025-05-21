#ifndef ERRORS_H
#define ERRORS_H

// #define DEBUG

#ifdef DEBUG
#define LOGGER(...) \
    fprintf(stdout, "(%s, %s) %d: ", __FILE__, __func__, __LINE__);     \
    fprintf(stdout, __VA_ARGS__);                                       \
    fprintf(stdout, "\n")
#endif
#ifndef DEBUG
#define LOGGER(...)
#endif

enum HtableError
{
    HTABLE_SUCCESS              = 0,
    HTABLE_MEMALLOC_ERROR       = 1,
    HTABLE_FILE_OPEN_ERROR      = 2,
    HTABLE_INSERT_ERROR         = 3,
    HTABLE_INIT_ERROR           = 4,
    HTABLE_UNKNOWN_ERROR        = 5,
    HTABLE_NULLPTR              = 6,

    HTABLE_FOUND                = 7,
    HTABLE_NOT_FOUND            = 8
};

enum ListError
{
    LST_SUCCESS,
    LST_MEMALLOC_ERROR,
    LST_FILE_OPEN_ERROR,
    LST_INSERT_ERROR,
    LST_INIT_ERROR,
    LST_UNKNOWN_ERROR
};

enum ListError ParseListError(enum ListError error);
enum HtableError ParseHtableError(enum HtableError error);

#endif
