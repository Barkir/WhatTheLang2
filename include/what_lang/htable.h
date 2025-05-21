#ifndef HTABLE_H
#define HTABLE_H

#include <xmmintrin.h>
#include <x86intrin.h>
#include <nmmintrin.h>
#include <immintrin.h>

const static int64_t HTABLE_BINS = 128;

typedef struct _htable
{
    size_t bins;
    size_t capacity;

    List ** table;
    char * bloom;

} Htable;

int HtableInit(Htable ** tab, size_t bins);

int HtableInsert(Htable * tab, const char * string);
int HtableFind(Htable * tab, const char * string);


Name * HtableNameFind(Htable * tab, Name * name);
int HtableNameInsert(Htable * tab, Name * name);

// int HtableOptFind(Htable * tab, const char * string, char * result);
// extern "C" int HtableOptFind(Htable * tab, const char * string, char * result);
int HtableOptInsert(Htable * tab, const char * string);

int HtableDestroy(Htable * tab);

int HtableDump(Htable * tab);
float HashLoadFactor(Htable * tab);

static inline int strcmp_asm(const char * el1, const char * el2)
{

    int mask = 0;

    __asm__ inline (
    ".intel_syntax noprefix             \n\t"
    "xor eax, eax                       \n\t"
    "vmovdqu ymm0, YMMWORD PTR [%1]     \n\t"
    "vptest ymm0,  YMMWORD PTR [%2]     \n\t"
    "setc %b0                           \n\t"
    "vzeroupper                         \n\t"
    ".att_syntax prefix                 \n\t"

    :"+&r" (mask)
    :"r" (el1), "r" (el2)
    : "ymm0", "cc"

    );

    return mask;

}


#endif
