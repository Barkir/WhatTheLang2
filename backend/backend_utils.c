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

const CmpOper OperArray[OPER_ARRAY_SIZE] =
{
    {.oper_enum = MORE,     .oper_str = "ja",   .oper_byte = JA_BYTE},
    {.oper_enum = LESS,     .oper_str = "jb",   .oper_byte = JB_BYTE},
    {.oper_enum = MORE_E,   .oper_str = "jae",  .oper_byte = JAE_BYTE},
    {.oper_enum = LESS_E,   .oper_str = "jbe",  .oper_byte = JBE_BYTE},
    {.oper_enum = EQUAL,    .oper_str = "je",   .oper_byte = JE_BYTE},
    {.oper_enum = N_EQUAL,  .oper_str = "jne",  .oper_byte = JNE_BYTE},
};

const WhatReg RegArray[] =
{
    {.reg = WHAT_REG_EAX,     .reg_xtnd = -1          ,  .reg_str = "rax"},
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

int _create_name_table(Node * root, Htable ** name_tab, NameTableCtx * ctx)
{
    assert(root);
    assert(name_tab);
    assert(ctx);

    if (NodeType(root) == VAR)
    {
        PARSER_LOG("Got VAR with name %s (stack_offset = %d)", NodeName(root), ctx->stack_offset);
        Name name = {.func_name = ctx->func_name, .name = NodeName(root), .type = VAR, .stack_offset = (ctx->stack_offset++)};
        if (HtableNameInsert(name_tab, &name) == HTABLE_REPEAT) ctx->stack_offset--;
    }

    else if (NodeType(root) == FUNC_EXT)
    {
        PARSER_LOG("Got FUNC_EXT with name %s (stack_offset = %d)", NodeName(root), ctx->stack_offset);
        Name name = {.name = NodeName(root), .type = FUNC_EXT, .func_name = NodeName(root)};
        HtableNameInsert(name_tab, &name);

    }
    else if (NodeType(root) == FUNC_INTER_CALL)
    {
        PARSER_LOG("Got FUNC_INTER_CALL with name %s (stack_offset = %d)", NodeName(root), ctx->stack_offset);
        Name name = {.name = NodeName(root), .type = FUNC_INTER_CALL, .func_name = ctx->func_name, .stack_offset=ctx->stack_offset};
        HtableNameInsert(name_tab, &name);
    }
    else if (NodeType(root) == FUNC_INTER_DEF)
    {
        PARSER_LOG("Got FUNC_INTER_DEF with name %s (stack_offset = %d)", NodeName(root), ctx->stack_offset);
        Name name = {.name = NodeName(root), .type = FUNC_INTER_DEF, .func_name = ctx->func_name};
        ctx->func_name = NodeName(root);

        name.name_array = calloc(DEFAULT_NAME_ARRAY_SIZE, sizeof(Name*));
        if (!name.name_array) return WHAT_MEMALLOC_ERROR;
        InitFuncParam(root->left, name_tab, &name, &ctx);

        HtableNameInsert(name_tab, &name);

        if (root->right) _create_name_table(root->right, name_tab, ctx);
        return WHAT_SUCCESS;
    }


    if (root->left)     _create_name_table(root->left,  name_tab, ctx);
    if (root->right)    _create_name_table(root->right, name_tab, ctx);
    return WHAT_SUCCESS;
}

Htable * CreateNameTable(Node * root)
{
    assert(root);

    Htable * name_tab = NULL;
    HtableInit(&name_tab, HTABLE_BINS);

    NameTableCtx nmt_ctx = {.tab = &name_tab, .func_name = GLOBAL_FUNC_NAME, .stack_offset = 0};

    _create_name_table(root, &name_tab, &nmt_ctx);
    return name_tab;
}


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

int isCmpOper(enum operations oper_enum)
{
    return oper_enum == MORE || oper_enum == LESS || oper_enum == MORE_E || oper_enum == LESS_E || oper_enum == EQUAL || oper_enum == N_EQUAL;
}

int isArithOper(enum operations oper_enum)
{
    return oper_enum == '+' || oper_enum == '-' || oper_enum == '/' || oper_enum == '*' || oper_enum == '=' ;
}

void BinCmpOper(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PrintNasmNode(root, ctx);

    int nodeVal = (int) NodeValue(root);

    if (root->left) _create_bin(ctx, tab, root->left );
    if (root->right)_create_bin(ctx, tab, root->right);

    EmitComparsion(ctx, tab, nodeVal);
}

void BinArithOper(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    int nodeVal = (int) NodeValue(root);

    if (nodeVal == '=')
    {
        PARSER_LOG("BinArithOper in '=' condition");
        if (root->right) _create_bin(ctx, tab, root->right);

        PrintNasmNode(root, ctx);

        assert(root->left);
        assert(GetVarFuncName(root->left, ctx));

        if (!strcmp(GetVarFuncName(root->left, ctx), GLOBAL_FUNC_NAME))
        {
            EmitPopReg      (ctx, Offset2EnumReg(GetVarOffset(root->left, ctx)));
            EmitPushXtendReg(ctx, WHAT_REG_R12);
            EmitAddRegVal   (ctx, WHAT_REG_R12, GetVarOffset(root->left, ctx) * 8, WHAT_XTEND_VAL);
            EmitMovRegReg   (ctx, WHAT_REG_R12, Offset2EnumReg(GetVarOffset(root->left, ctx)), WHAT_XTEND_REG, WHAT_MEM1);
            EmitPopXtendReg (ctx, WHAT_REG_R12);
        }
        else
        {
            EmitPopReg(ctx, Offset2EnumReg(GetVarParam(root->left, ctx)));
        }

        return;
    }

    if (root->left) _create_bin(ctx, tab, root->left );
    if (root->right)_create_bin(ctx, tab, root->right);


    PrintNasmNode(root, ctx);
    if (nodeVal == '+')
    {
        EmitPopXtendReg (ctx, WHAT_REG_R14);
        EmitPopXtendReg (ctx, WHAT_REG_R15);
        EmitAddRegReg   (ctx, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);
        EmitPushXtendReg(ctx, WHAT_REG_R14);
        return;
    }

    else if (nodeVal == '-')
    {
        EmitPopXtendReg (ctx, WHAT_REG_R14);
        EmitPopXtendReg (ctx, WHAT_REG_R15);
        EmitSubRegReg   (ctx, WHAT_REG_R15, WHAT_REG_R14, WHAT_XTEND_XTEND);
        EmitPushXtendReg(ctx, WHAT_REG_R15);
        return;
    }

    else if (nodeVal == '*')
    {

        EmitPopXtendReg(ctx, WHAT_REG_R14);
        EmitPopReg     (ctx, WHAT_REG_EAX);
        EmitMulXtendReg(ctx, WHAT_REG_R14);
        EmitPushReg    (ctx, WHAT_REG_EAX);
        return;
    }


    else if (nodeVal == '/')
    {

        EmitPopXtendReg(ctx, WHAT_REG_R14);
        EmitPopReg     (ctx, WHAT_REG_EAX);
        EmitDivXtendReg(ctx, WHAT_REG_R14);
        EmitPushReg    (ctx, WHAT_REG_EAX);
        return;
    }

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
    ctx->if_cond = 0;
    ctx->while_cond = 1;
    if (root) _create_bin(ctx, tab, root);
}

int BinIf(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PARSER_LOG("PROCESSING IF");
    PrintNasmNode(root, ctx);

    int local_if  = IF_COUNT;
    ctx->if_count = IF_COUNT;
    IF_COUNT++;

    Name * locals_if = InitLocalName(LABEL_SIZE);                               // Structure where local label
    assert(locals_if);                                                          // name will contain

    if (root->left) BinIfBlock(ctx, tab, root->left);
    PARSER_LOG("PROCESSED IF BLOCK... if_count = %d", ctx->if_count);

    InsertOffsetToLabel(ctx, locals_if, tab, "IF", local_if);

    fprintf(ctx->file, "IF%d:\n", ctx->if_count);

    EmitPushImm32(ctx, 0);

    EmitCmpRegsBlock(ctx);                                                      // Comparing values, popping them to r14 and r15

    char * cond   = EmitCondJmp(ctx, NULL, "jne COND",   ctx->if_count, JNE_BYTE, 0); // >>>>>>>>>>>>>>>>>>>>>>> v
    char * if_end = EmitCondJmp(ctx, NULL, "jmp IF_END", ctx->if_count, JMP_BYTE, 0); // >>>>>>>>>>>>>>>> v      v
                                                                                //                        v      v
    InsertOffsetToPtr(ctx, cond, "COND", ctx->if_count); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< v      v
                                                                                //                               v
    ctx->if_count++;                                                            //                               v
    if (root->right) BinIfBlock(ctx, tab, root->right);                         //                               v
                                                                                //                               v
    InsertOffsetToPtr  (ctx, if_end,"IF_END", local_if); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< v
    InsertOffsetToLabel(ctx, locals_if, tab, "IF_END", local_if);
    return WHAT_SUCCESS;

}

int BinWhile(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PARSER_LOG("PROCESSING WHILE");
    PrintNasmNode(root, ctx);

    int local_while = WHILE_COUNT;
    ctx->while_count = WHILE_COUNT;
    WHILE_COUNT++;

    Name * locals_while = InitLocalName(LABEL_SIZE);                           // Structure where local label
    assert(locals_while);                                                      // will contain

    fprintf(ctx->file, "WHILE%d:\n", ctx->while_count);
    const char * while_ptr = ctx->buf - 5;                                     // -5 is here to include previous instructions

    if (root->left) BinWhileBlock(ctx, tab, root->left);
    PARSER_LOG("PROCESSED WHILE BLOCK");


    InsertOffsetToLabel(ctx, locals_while, tab, "WHILE_FALSE", local_while);
    fprintf(ctx->file, "WHILE_FALSE%d:\n", ctx->while_count);

    EmitPushImm32(ctx, 0);

    EmitCmpRegsBlock(ctx);                                                      // Comparing values, popping them to r14 and r15

    char * while_end_ptr = EmitCondJmp(ctx, NULL, "je WHILE_END", local_while, JE_BYTE, 0); // >>>>> v
                                                                                      //       v
    fprintf(ctx->file, "WHILE_TRUE%d:\n", ctx->while_count);                          //       v
    InsertOffsetToLabel(ctx, locals_while, tab, "WHILE_TRUE", local_while);           // >>>>> v >>> v
                                                                                      //       v     v
    if (root->right) BinWhileBlock(ctx, tab, root->right);                            //       v     v
                                                                                      //       v     v
    fprintf(ctx->file, "jmp WHILE%d\n", local_while); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< v <<< v
    EmitLongJmp(ctx, (int)((while_ptr) - ctx->buf));                                  //       v
                                                                                      //       v
    InsertOffsetToPtr(ctx, while_end_ptr, "WHILE_END", local_while); // <<<<<<<<<<<<<<<<<<<<<<<v
    return WHAT_SUCCESS;
}

int BinFuncExt(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PrintNasmNode(root, ctx);
    int nodeVal = (int) NodeValue(root);

    if (nodeVal == PRINT)
    {
        if (root->left) _create_bin(ctx, tab, root->left);
        EmitPrint(ctx);
    }

    else if (nodeVal == INPUT)
    {
        EmitInput(ctx);
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


