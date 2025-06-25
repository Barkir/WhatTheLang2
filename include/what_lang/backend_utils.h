#ifndef BACKEND_UTILS_H
#define BACKEND_UTILS_H

void PrintNasmNode(Node * root, BinCtx * ctx);

int WriteIOLib(BinCtx * ctx);

const enum Registers Offset2EnumReg(int adr);
const char * Offset2StrReg(int adr, int xtnd);
const char * EnumReg2Str(int reg, int xtnd);

Name * InitLocalName(size_t sz);
int InitFuncParam(Node * root, Htable ** name_tab, Name * function, NameTableCtx ** ctx);
void InsertOffsetToLabel(BinCtx * ctx, Name * local, Htable ** tab, const char * label_str, int local_count);
void InsertOffsetToPtr(BinCtx * ctx, char * cond, const char * cond_str, int cond_count);
void InsertLabelToHtable(BinCtx * ctx, Htable ** tab, Name * locals, const char * label, int label_count);

int BinIfBlock(BinCtx * ctx, Htable ** tab, Node * root);
int BinWhileBlock(BinCtx * ctx, Htable ** tab, Node * root);

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


#endif
