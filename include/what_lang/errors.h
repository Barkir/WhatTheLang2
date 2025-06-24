#ifndef ERRORS_H
#define ERRORS_H

#define DEBUG

#define SYNTAX_ERROR(exp, real)                     \
    {                                               \
        SyntaxError(exp, real, __func__, __LINE__); \
    }                                               \

#define NODE_VALUE(node)                            \
{                                                   \
    fprintf(stderr, "%s %d", __func__, __LINE__);   \
    NodeValue(node);                                \
}                                                   \

#ifdef DEBUG
#define PARSER_LOG(...)                                                          \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }                                                                            \

#define VERIFY_PTRS(...)                                                        \
        void* _ptrs[] = {__VA_ARGS__};                                          \
        for (size_t _i = 0; _i < sizeof(_ptrs)/sizeof(_ptrs[0]); _i++)          \
            assert(_ptrs[_i] != NULL);                                          \

#else
#define PARSER_LOG(...)
#endif

enum WhatTheErrors
{
    WHAT_SUCCESS,
    WHAT_MEMALLOC_ERROR,
    WHAT_FILEOPEN_ERROR,
    WHAT_FILEWRITE_ERROR,
    WHAT_NULLPOINTER_ERROR,
    WHAT_NOLABEL_ERROR,
    WHAT_VARTABLE_ERROR,
    WHAT_FUNCTABLE_ERROR,
    WHAT_NOTFOUND_ERROR
};

enum errors
{
    SUCCESS,
    ALLOCATE_MEMORY_ERROR,
    MEMCPY_ERROR,
    FOPEN_ERROR,
    FCLOSE_ERROR
};

void SyntaxError(char exp, char real, const char * func, int line);

#endif
