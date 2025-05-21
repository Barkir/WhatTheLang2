#ifndef LIST_H
#define LIST_H

#include <immintrin.h>


typedef int64_t (*CompareFunc) (const void *, const void *);

typedef struct _list
{
    char * elem;
    Name * name;
    __m128i elem16;
    struct _list * nxt;
    CompareFunc cmp;
} List;

int ListInsert(List * lst, const char * str);
int ListCtor(List * lst);
int64_t ListCompare(const void * el1, const void * el2);

#endif
