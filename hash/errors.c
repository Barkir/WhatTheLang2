#include <stdlib.h>
#include <stdio.h>

#include <xmmintrin.h>
#include <x86intrin.h>
#include <nmmintrin.h>
#include <immintrin.h>

#include "what_lang/nametable.h"
#include "what_lang/hashtable_errors.h"
#include "what_lang/list.h"
#include "what_lang/htable.h"


enum ListError ParseListError(enum ListError error)
{
    switch(error)
    {
        case LST_MEMALLOC_ERROR:    LOGGER("LST_MEMALLOC_ERROR");
                                    break;

        case LST_SUCCESS:           LOGGER("LST_SUCCESS");
                                    return LST_SUCCESS;

        default:                    LOGGER("LST_UNKNOWN_ERROR");
                                    return LST_UNKNOWN_ERROR;
    }

    return LST_UNKNOWN_ERROR;
}

enum HtableError ParseHtableError(enum HtableError error)
{
    switch(error)
    {
        case HTABLE_MEMALLOC_ERROR: LOGGER("HTABLE_MEMALLOC_ERROR");
                                    return HTABLE_MEMALLOC_ERROR;

        case HTABLE_SUCCESS:        LOGGER("HTABLE_SUCCESS");
                                    return HTABLE_SUCCESS;

        default:                    LOGGER("HTABLE_UNKNOWN_ERROR");
                                    return HTABLE_UNKNOWN_ERROR;
    }

    return HTABLE_UNKNOWN_ERROR;
}
