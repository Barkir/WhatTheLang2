
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "what_lang/nametable.h"
#include "what_lang/list.h"
#include "what_lang/hashtable_errors.h"


int ListCtor(List * lst)
{
    lst = (List*) calloc(sizeof(List), 1);
    lst->cmp = ListCompare;

    if (!lst) return LST_MEMALLOC_ERROR;

    return LST_SUCCESS;
}

int ListInsert(List * lst, const char * str)
{

    while (lst)
    {
        lst = lst->nxt;
    }

    lst = (List*) calloc(sizeof(List), 1);
    if (!lst) return LST_MEMALLOC_ERROR;
    lst->elem = strdup(str);

    return LST_SUCCESS;
}

int64_t ListCompare(const void * el1, const void * el2)
{
    return strcmp((const char*) el1, (const char*) el2);
}



