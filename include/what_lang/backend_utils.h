#ifndef BACKEND_UTILS_H
#define BACKEND_UTILS_H


#define _CREATE_BIN_WHILE                                               \
    ctx->if_cond = 0;                                                   \
    ctx->while_cond = 1;                                                \
    _create_bin(buf, tab, root->left, ctx)


void PrintNasmNode(Node * root, BinCtx * ctx);

int WriteIOLib(BinCtx * ctx);

const enum Registers Offset2EnumReg(int adr);
const char * Offset2StrReg(int adr, int xtnd);
const char * EnumReg2Str(int reg, int xtnd);

Name * CreateVarTable(Node * root);
Name * CreateFuncTable(Node * root);


Htable * CreateNameTable(Node * root);

const char * GetVarName(Node * root);
int GetVarParam(Node * root, BinCtx * ctx);
int GetVarOffset(Node * root, BinCtx * ctx);
const char * GetVarFuncName(Node * root, BinCtx * ctx);
char ** GetFuncAdrArr(Node * root, BinCtx * ctx);
Name ** GetFuncNameArray(Node * root, BinCtx * ctx);
int GetFuncAdrArrCap(Node * root, BinCtx * ctx);

// Compare functions (used in BinCmpOper, etc.)
int             isCmpOper   (enum operations oper_enum);
int             isArithOper (enum operations oper_enum);
const char *    CmpStr      (enum operations oper_enum);
char            CmpByte     (enum operations oper_enum);

//  Functions for generating binary code from if, while or opers (yep, i know there is bad parameters, fixing it soon...)
void            BinCmpOper  (BinCtx * ctx, Htable ** tab, Node * root);
void            BinArithOper(BinCtx * ctx, Htable ** tab, Node * root);
int             BinWhile    (BinCtx * ctx, Htable ** tab, Node * root);
int             BinIf       (BinCtx * ctx, Htable ** tab, Node * root);
int             BinFuncExt  (BinCtx * ctx, Htable ** tab, Node * root);
int             AddFuncAdr(BinCtx * ctx, Node * root);

typedef struct _cmp_oper
{
    enum operations oper_enum;
    const char * oper_str;
    const char oper_byte;

} CmpOper;

typedef struct _what_reg
{
    enum Registers reg;
    enum Registers reg_xtnd;
    const char * reg_str;

} WhatReg;

typedef struct _nametable_ctx
{
    Htable ** tab;
    const char * func_name;
    int stack_offset;

} NameTableCtx;

#endif
