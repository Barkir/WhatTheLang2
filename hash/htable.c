#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <xmmintrin.h>
#include <x86intrin.h>
#include <nmmintrin.h>
#include <immintrin.h>
#include <wmmintrin.h>

#include "what_lang/nametable.h"
#include "what_lang/list.h"
#include "what_lang/htable.h"
#include "what_lang/hashtable_errors.h"
#include "what_lang/IO.h"
#include "what_lang/crc32.h"
#include "what_lang/errors.h"

const char * GLOBAL_FUNC_NAME_H = "GLOBAL_FUNC";


int HtableInit(Htable ** tab, size_t bins)
{
    *tab = (Htable*) calloc(BUF_LEN, sizeof(Htable));
    if (!tab) return ParseHtableError(HTABLE_MEMALLOC_ERROR);

    (*tab)->bins = bins;


    (*tab)->table = (List**) calloc(bins, sizeof(List*));
    if (!(*tab)->table) return ParseHtableError(HTABLE_MEMALLOC_ERROR);

    (*tab)->bloom = (char*) calloc(bins, 1);
    if (!(*tab)->bloom) return ParseHtableError(HTABLE_MEMALLOC_ERROR);


    return HTABLE_SUCCESS;
}

int HtableDestroy(Htable * tab)
{
    for (int i = 0; i < tab->bins; i++)
    {
        List * lst = tab->table[i];
        while (lst)
        {
            List * e = lst;
            lst = lst->nxt;
            free(e->elem);
            free(e);
        }
    }

    free(tab->table);
    free(tab);
    return HTABLE_SUCCESS;
}

int HtableInsert(Htable * tab, const char * string)
{
    size_t ind = crc32_naive(string, strlen(string), CRC32INIT) % tab->bins;
    for (List * lst = tab->table[ind]; lst; lst=lst->nxt)
    {

    }

    List * n = (List*) calloc(1, sizeof(List));
    if (!n) return HTABLE_MEMALLOC_ERROR;

    n->elem = (char*) calloc(BUF_LEN, BUF_LEN);
    if (!n->elem) return HTABLE_MEMALLOC_ERROR;

    memcpy(n->elem, string, strlen(string) + 1);

    n->nxt = tab->table[ind];
    tab->table[ind] = n;


    return HTABLE_SUCCESS;
}

int HtableLabelInsert(Htable ** tab, Name * name)
{
    PARSER_LOG("Inserting name %s with offset %p", name->local_func_name, name->offset);
    size_t ind = crc32_naive(name->local_func_name, strlen(name->local_func_name), CRC32INIT) % (*tab)->bins;
    PARSER_LOG("bin %ld", ind);


    for (List * lst = (*tab)->table[ind]; lst; lst=lst->nxt)
    {
        PARSER_LOG("inserting to list %p", lst);
        if (!strcmp(lst->name->local_func_name, name->local_func_name))
        {
            PARSER_LOG("ALREADY THERE!");
            return HTABLE_SUCCESS;
        }
    }

    List * n = (List*) calloc(1, sizeof(List));
    if (!n) return HTABLE_MEMALLOC_ERROR;

    n->name = (Name*) calloc(1, sizeof(Name));
    if (!n->name) return HTABLE_MEMALLOC_ERROR;

    n->name->local_func_name = strdup(name->local_func_name);
    n->name->offset = name->offset;
    PARSER_LOG("copied %s to list %p", n->name->local_func_name, n);

    n->nxt = (*tab)->table[ind];
    (*tab)->table[ind] = n;

    return HTABLE_SUCCESS;
}

int HtableOptInsert(Htable * tab, const char * string)
{
    if (!tab) return HTABLE_NULLPTR;
    if (!string) return HTABLE_NULLPTR;

    int hash = crc32_intinsic(string);

// Bloom filter implementation
// ----------------------------
    unsigned char * bytes =  (unsigned char*) &hash;
    tab->bloom[bytes[0]] = 1;       //LOGGER("%d", bytes[0]);
    tab->bloom[bytes[1]] = 1;       //LOGGER("%d", bytes[1]);
    tab->bloom[bytes[2]] = 1;       //LOGGER("%d", bytes[2]);
    tab->bloom[bytes[3]] = 1;       //LOGGER("%d", bytes[3]);
// ----------------------------

    int bin = hash % tab->bins;

    for (List * lst = tab->table[bin]; lst; lst=lst->nxt)
    {
        if (strcmp_asm(lst->elem, string))
            return HTABLE_SUCCESS;
    }

    List * n = (List*) calloc(1, sizeof(List));
    if (!n) return HTABLE_MEMALLOC_ERROR;

    n->elem = (char*) calloc(BUF_LEN, 1);
    if (!n->elem) return HTABLE_MEMALLOC_ERROR;

    memcpy(n->elem, string, strlen(string) + 1);

    n->nxt = tab->table[bin];
    tab->table[bin] = n;

    return HTABLE_SUCCESS;
}

int HtableFind(Htable * tab, const char * string)
{
    int bin = crc32_naive(string, strlen(string), CRC32INIT) % tab->bins;

    for (List * lst = tab->table[bin]; lst; lst = lst->nxt)
    {
        if (!strcmp(string, lst->elem)) return HTABLE_FOUND;
    }

    return HTABLE_NOT_FOUND;
}

Name * HtableLabelFind(Htable * tab, Name * name)
{
    PARSER_LOG("FINDING LABEL %s", name->local_func_name);
    size_t bin = crc32_naive(name->local_func_name, strlen(name->local_func_name), CRC32INIT) % tab->bins;
    PARSER_LOG("bin %ld", bin);

    for (List * lst = tab->table[bin]; lst; lst = lst->nxt)
    {
        PARSER_LOG("finding in list %p", lst);
        PARSER_LOG("%s vs %s", lst->name->local_func_name, name->local_func_name)
        if (!strcmp(lst->name->local_func_name, name->local_func_name)) return lst->name;
    }

    return NULL;
}

Name * HtableNameFind(Htable * tab, Name * name)
{
    PARSER_LOG("FINDING NAME %s", name->name);
    size_t bin = crc32_naive(name->name, strlen(name->name), CRC32INIT) % tab->bins;

    for (List * lst = tab->table[bin]; lst; lst = lst->nxt)
    {
        PARSER_LOG("finding in list %p", lst);
        if (!strcmp(lst->name->name, name->name) && lst->name->type == name->type)
        {
            PARSER_LOG("FOUND Name %s", lst->name->name);
            return lst->name;
        }
    }

    return NULL;
}

int HtableNameInsert(Htable ** tab, Name * name)
{
    PARSER_LOG("Inserting name %s with type %d in hash table", name->name, name->type);
    size_t ind = crc32_naive(name->name, strlen(name->name), CRC32INIT) % (*tab)->bins;
    PARSER_LOG("bin = %ld", ind);

    for (List * lst = (*tab)->table[ind]; lst; lst=lst->nxt)
    {
        PARSER_LOG("inserting to list %p with name %s, type %d", lst, lst->name->name, lst->name->type);
        if (!strcmp(lst->name->name, name->name) && (lst->name->type == name->type))
        {
            if (!(strcmp(lst->name->func_name, GLOBAL_FUNC_NAME_H)))
            {
                PARSER_LOG("GLOBAL VARIABLE WITH SAME NAME. STANDART CONSIDERS IT AS A GLOBAL VARIABLE");
                return HTABLE_REPEAT;
            }
            else if (!(strcmp(lst->name->func_name, name->func_name)))
            {
                PARSER_LOG("LOCAL VARIABLE IN FUNCTION. ALREADY EXISTS");
                return HTABLE_REPEAT;
            }
        }
    }

    PARSER_LOG("Adding new element to bin...");
    List * n = (List*) calloc(1, sizeof(List));
    if (!n) return HTABLE_MEMALLOC_ERROR;

    n->name = (Name*) calloc(1, sizeof(Name));
    if (!n->name) return HTABLE_MEMALLOC_ERROR;

    n->name->name           = strdup(name->name);
    n->name->func_name      = strdup(name->func_name);
    n->name->type           = name->type;
    n->name->stack_offset   = name->stack_offset;
    n->name->name_array     = name->name_array;
    n->name->adr_array      = calloc(32, sizeof(char*));

    PARSER_LOG("Added new element to bin...");

    n->nxt = (*tab)->table[ind];
    (*tab)->table[ind] = n;

    return HTABLE_SUCCESS;
}

// int HtableOptFind(Htable * tab, const char * string, char * result)
// {
//     int bin = crc32_intinsic(string) % tab->bins;
//     for (List * lst = tab->table[bin]; lst; lst = lst->nxt)
//     {
//         // LOGGER("%ld, %ld", (size_t) string % 32, (size_t) lst->elem % 32);
//         if (strcmp_asm(string, lst->elem))
//         {
//             // LOGGER("FOUND WORD %s, bin = %d", string, bin);
//             return HTABLE_FOUND;
//         }
//     }
//
//     // LOGGER("NOT FOUND WORD %s, bin = %d", string, bin);
//
//     return HTABLE_NOT_FOUND;
// }

int HtableDump(Htable * tab)
{
    FILE * file = fopen("log.dmp", "w+");
    if (!file) return  HTABLE_FILE_OPEN_ERROR;
    fprintf(file, "<<<LOG FILE OF HASH TABLE>>>\n");

    fprintf(file, "Load factor = %f\n", HashLoadFactor(tab));

    for (int bins = 0; bins < tab->bins; bins++)
    {
        fprintf(file, "[BIN %d]", bins);
        fprintf(file, "----------------------------------\n");
        for (List * lst = tab->table[bins]; lst; lst=lst->nxt)
        {
            if (lst->name) fprintf(file, "\t type = %d, name = %s stack_offset = %d, param = %d, func_name = %s\n", lst->name->type, lst->name->name, lst->name->stack_offset, lst->name->param, lst->name->func_name);
            if (lst->name->name_array)
            {
                fprintf(file, "\t NameArray = {");
                for (int i = 0; lst->name->name_array[i]; i++)
                    fprintf(file, "%s ", lst->name->name_array[i]->name);

                fprintf(file, "}\n");
            }
            fprintf(file, "----------------------------------\n");
        }
    }

    return HTABLE_SUCCESS;
}

float HashLoadFactor(Htable * tab)
{
    float lst_count = 0;
    for (int i = 0; i < tab->bins; i++)
    {
        for (List * lst = tab->table[i]; lst; lst = lst->nxt, lst_count++)
        {}
    }

    return lst_count / tab->bins;
}
