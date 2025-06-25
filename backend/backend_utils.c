#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "what_lang/constants.h"
#include "what_lang/nasm2elf.h"
#include "what_lang/tree.h"
#include "what_lang/nametable.h"
#include "what_lang/list.h"
#include "what_lang/htable.h"
#include "what_lang/parser.h"
#include "what_lang/backend.h"
#include "what_lang/backend_utils.h"
#include "what_lang/emitters.h"
#include "what_lang/errors.h"
#include "what_lang/hashtable_errors.h"
#include "what_lang/tokenizer.h"

void PrintNasmNode(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    if (NodeType(root) != SEP_SYMB)
        fprintf(ctx->file, "\n ;%s :: %s :: %s\n", NodeName(root), NodeType2Str(root), TokenArray[(int) NodeValue(root)].graphviz_str);
}

int NameArrayAddElem(Name * name, Name * elem)
{
    assert(name);
    assert(elem);

    Name ** array = name->name_array;
    for (int i = 0; i < DEFAULT_NAME_ARRAY_SIZE; i++)
    {
        if (!array[i])
        {
            array[i] = elem;
            array[i]->param = i;
            return WHAT_SUCCESS;
        }
    }
    return WHAT_SUCCESS;
}

int AddFuncAdr(BinCtx * ctx, Node * root)
{
    assert(root);
    assert(ctx);

    Name func = {.name = NodeName(root), .type = FUNC_INTER_DEF};
    Name * found_func = HtableNameFind(ctx->names, &func);
    for (int i = 0; i < ADR_ARRAY_SIZE; i++)
    {
        if (!found_func->adr_array[i])
        {
            found_func->adr_array[i] = ctx->buf;
            PARSER_LOG("adr_array[%d] = %p", i, found_func->adr_array[i]);
            found_func->adr_array_cap++;
            PARSER_LOG("cap = %d", found_func->adr_array_cap);
            return WHAT_SUCCESS;
        }
    }

    return WHAT_NOTFOUND_ERROR;
}

int InitFuncParam(Node * root, Htable ** name_tab, Name * function, NameTableCtx ** ctx)
{
    assert(root);
    assert(name_tab);
    assert(function);
    assert(ctx);

    Name param_name = {.name = NodeName(root), .type = VAR, .func_name = (*ctx)->func_name, .stack_offset = ((*ctx)->stack_offset++)};

    HtableNameInsert(name_tab, &param_name);
    NameArrayAddElem(function, HtableNameFind(*name_tab, &param_name));
    if (root->left) InitFuncParam(root->left, name_tab, function, ctx);

    return WHAT_SUCCESS;
}

int isCmpOper(enum operations oper_enum)
{
    return oper_enum == MORE || oper_enum == LESS || oper_enum == MORE_E || oper_enum == LESS_E || oper_enum == EQUAL || oper_enum == N_EQUAL;
}

int isArithOper(enum operations oper_enum)
{
    return oper_enum == '+' || oper_enum == '-' || oper_enum == '/' || oper_enum == '*' || oper_enum == '=' ;
}

Name * InitLocalName(size_t sz)
{
    Name * local = calloc(1, sizeof(Name));
    if (!local) return NULL;

    local->local_func_name = calloc(sz, sizeof(char));
    if (!local->local_func_name) return NULL;

    return local;
}

void InsertLabelToHtable(BinCtx * ctx, Htable ** tab, Name * locals, const char * label, int label_count)
{
    sprintf(locals->local_func_name, "%s%d", label, label_count);
    locals->offset = ctx->buf - 1;
    HtableLabelInsert(tab, locals);
}

void InsertOffsetToLabel(BinCtx * ctx, Name * local, Htable ** tab, const char * label_str, int local_count)
{
    assert(ctx);
    assert(local);
    assert(tab);
    assert(label_str);

    sprintf(local->local_func_name, "%s%d", label_str, local_count);
    Name * label = HtableLabelFind(*tab, local);
    assert(label);
    *(label->offset) = (int8_t) (ctx->buf - (label->offset + 1));
}

void InsertOffsetToPtr(BinCtx * ctx, char * cond, const char * cond_str, int cond_count)
{
    assert(ctx);
    assert(cond);
    assert(cond_str);

    fprintf(ctx->file, "%s%d:\n", cond_str, cond_count);
    *cond = ctx->buf - (cond + 1);
}

int BinIfBlock(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(ctx);
    assert(tab);
    assert(root);

    ctx->if_cond = 1;
    ctx->while_cond = 0;
    if (root) _create_bin(ctx, tab, root);
}

int BinWhileBlock(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(ctx);
    assert(tab);
    assert(root);

    ctx->if_cond = 0;
    ctx->while_cond = 1;
    if (root) _create_bin(ctx, tab, root);
}

const CmpOper OperArray[OPER_ARRAY_SIZE] =                              // Used in:
{                                                                       // CmpByte
    {.oper_enum = MORE,     .oper_str = "ja",   .oper_byte = JA_BYTE},  // CmpStr
    {.oper_enum = LESS,     .oper_str = "jb",   .oper_byte = JB_BYTE},
    {.oper_enum = MORE_E,   .oper_str = "jae",  .oper_byte = JAE_BYTE},
    {.oper_enum = LESS_E,   .oper_str = "jbe",  .oper_byte = JBE_BYTE},
    {.oper_enum = EQUAL,    .oper_str = "je",   .oper_byte = JE_BYTE},
    {.oper_enum = N_EQUAL,  .oper_str = "jne",  .oper_byte = JNE_BYTE},
};


char CmpByte(enum operations oper_enum)
{
    for (int i = 0; i < OPER_ARRAY_SIZE; i++)
    {
        if ((char) OperArray[i].oper_enum == oper_enum)
        {
            PARSER_LOG("returning %d", OperArray[i].oper_byte);
            return OperArray[i].oper_byte;
        }
    }

    return 0;
}

const char * CmpStr(enum operations oper_enum)
{
    PARSER_LOG("%d in CmpStr", oper_enum)
    for (int i = 0; i < OPER_ARRAY_SIZE; i++)
    {
        if (OperArray[i].oper_enum == oper_enum)
        {
            PARSER_LOG("returning %s", OperArray[i].oper_str);
            return OperArray[i].oper_str;
        }
    }

    return NULL;
}

const WhatReg RegArray[] =                                                      // used in:
{                                                                               // Offset2StrReg
    {.reg = WHAT_REG_EAX,     .reg_xtnd = -1          ,  .reg_str = "rax"},     // EnumReg2Str
    {.reg = WHAT_REG_EBX,     .reg_xtnd = -1          ,  .reg_str = "rbx"},
    {.reg = WHAT_REG_EBP,     .reg_xtnd = -1          ,  .reg_str = "rbp"},
    {.reg = WHAT_REG_ECX,     .reg_xtnd = -1          ,  .reg_str = "rcx"},
    {.reg = WHAT_REG_EDX,     .reg_xtnd = -1          ,  .reg_str = "rdx"},
    {.reg = WHAT_REG_EDI,     .reg_xtnd = -1          ,  .reg_str = "rdi"},
    {.reg = WHAT_REG_ESI,     .reg_xtnd = -1          ,  .reg_str = "rsi"},
    {.reg = WHAT_REG_ESP,     .reg_xtnd = -1          ,  .reg_str = "rsp"},
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R8 ,  .reg_str = "r8" },
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R9 ,  .reg_str = "r9" },
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R10,  .reg_str = "r10"},
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R11,  .reg_str = "r11"},
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R12,  .reg_str = "r12"},
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R13,  .reg_str = "r13"},
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R14,  .reg_str = "r14"},
    {.reg = -1          ,     .reg_xtnd = WHAT_REG_R15,  .reg_str = "r15"}
};

const char * Offset2StrReg(int adr, int xtnd)
{
    PARSER_LOG("Calling Offset2StrReg");
    if (xtnd)
    {
        for (int i = 0; i < REG_ARRAY_SIZE; i++)
        {
            if (RegArray[i].reg_xtnd == adr) return RegArray[i].reg_str;
        }
    }

    switch(adr % DEFAULT_REG_NUMBER)
    {
            case 0:     return "rbx";
            case 1:     return "rcx";
            case 2:     return "rdx";
            case 3:     return "rsi";
            case 4:     return "rdi";
    }

    return NULL;
}

const enum Registers Offset2EnumReg(int adr)
{
    switch(adr % DEFAULT_REG_NUMBER)
    {
        case 0:     return WHAT_REG_EBX;
        case 1:     return WHAT_REG_ECX;
        case 2:     return WHAT_REG_EDX;
        case 3:     return WHAT_REG_ESI;
        case 4:     return WHAT_REG_EDI;
        default:    return WHAT_REG_UNK;
    }

    return WHAT_REG_UNK;

}

const char * EnumReg2Str(int reg, int xtnd)
{
    if (xtnd)
    {
        for (int i = 0; i < REG_ARRAY_SIZE; i++)
            if (RegArray[i].reg_xtnd == reg) return RegArray[i].reg_str;
    }

    for (int i = 0; i < REG_ARRAY_SIZE; i++)
        if (RegArray[i].reg == reg) return RegArray[i].reg_str;

    return NULL;
}

const char * GetVarFuncName(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    Name root_name = {.name = NodeName(root), .type  = NodeType(root)};
    return HtableNameFind(ctx->names, &root_name)->func_name;
}

char ** GetFuncAdrArr(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    Name root_name = {.name = NodeName(root), .type = FUNC_INTER_DEF};
    return HtableNameFind(ctx->names, &root_name)->adr_array;
}

int GetFuncAdrArrCap(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    Name root_name = {.name = NodeName(root), .type = FUNC_INTER_DEF};
    return HtableNameFind(ctx->names, &root_name)->adr_array_cap;
}

int GetVarOffset(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    Name root_name = {.name = NodeName(root), .type = NodeType(root)};
    return HtableNameFind(ctx->names, &root_name)->stack_offset;
}

int GetVarParam(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    Name root_name = {.name = NodeName(root), .type = NodeType(root)};
    return HtableNameFind(ctx->names, &root_name)->param;
}

Name ** GetFuncNameArray(Node * root, BinCtx * ctx)
{
    assert(root);
    assert(ctx);

    Name func = {.name = NodeName(root), .type = FUNC_INTER_DEF};
    return HtableNameFind(ctx->names, &func)->name_array;
}


int WriteIOLib(BinCtx * ctx)
{
    assert(ctx);

    FILE * IOlibRaw = fopen(IOLIB_PATH, "rb");
    if (!IOlibRaw) return WHAT_FILEOPEN_ERROR;

    size_t sz = FileSize(IOlibRaw);
    if (((BUF_DEF_SIZE - IOLIB_OFFSET) - sz) < 0) return WHAT_BUFOVERFLOW_ERROR;
    if (fread(ctx->buf_ptr + IOLIB_OFFSET, sizeof(char), sz, IOlibRaw) != sz) return WHAT_FILEREAD_ERROR;
    fclose(IOlibRaw);

    return WHAT_SUCCESS;
}


