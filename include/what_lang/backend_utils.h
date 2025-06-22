#ifndef BACKEND_UTILS_H
#define BACKEND_UTILS_H

static int IF_COUNT = 0;
static int WHILE_COUNT = 0;
static int ADR_COUNT = 0;
const static int DEFAULT_REG_NUMBER = 5;


#define _CREATE_BIN_WHILE                       \
    ctx->if_cond = 0;                           \
    ctx->while_cond = 1;                        \
    _create_bin(buf, tab, root->left, ctx)      \


const static char * GLOBAL_FUNC_NAME = "GLOBAL_FUNC";

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

// Compare functions (used in BinCmpOper, etc.)
int             isCmpOper   (enum operations oper_enum);
int             isArithOper (enum operations oper_enum);
const char *    CmpStr      (enum operations oper_enum);
char            CmpByte     (enum operations oper_enum);

//  Functions for generating binary code from if, while or opers (yep, i know there is bad parameters, fixing it soon...)
void            BinCmpOper  (char ** buf, Htable ** tab, Node * root, BinCtx * ctx);
void            BinArithOper(char ** buf, Htable ** tab, Node * root, BinCtx * ctx);
int             BinWhile    (char ** buf, Htable ** tab, Node * root, BinCtx * ctx);
int             BinIf       (char ** buf, Htable ** tab, Node * root, BinCtx * ctx);
int             BinFuncExt     (char ** buf, Htable ** tab, Node * root, BinCtx * ctx);


int AddFuncAdr(char ** buf, Node * root, BinCtx * ctx);

static const char *     NASM_TOP =  "%include \"iolib/iolib.asm\"   \n"
                                    "section .text                  \n"
                                    "global _start                  \n"
                                    "_start:                        \n"
                                    "mov r13, stack4calls           \n"
                                    "mov r12, array4var             \n";

static const char *     NASM_BTM = "section .data                  \n"
                                    "stack4calls times 128 * 8 db 0 \n"
                                    "array4var   times 16 * 4  db 0 \n"
                                    "section .text                  \n";

static const char * RET_PUSH_STR =  "; pushing return address to stack  \n"
                                    "pop r14                            \n"
                                    "mov [r13], r14                     \n"
                                    "add r13, 8                         \n";


static const char * RET_POP_STR =   "; popping return address to stack  \n"
                                    "sub r13, 8                         \n"
                                    "mov r14, [r13]                     \n"
                                    "push r14                           \n"
                                    "ret                                \n";

typedef struct _cmp_oper
{
    enum operations oper_enum;
    const char * oper_str;
    char oper_byte;

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

#define OPER_ARRAY_SIZE 6
#define REG_ARRAY_SIZE  16

const static CmpOper OperArray[OPER_ARRAY_SIZE] =
{
    {.oper_enum = MORE,     .oper_str = "ja",   .oper_byte = JA_BYTE},
    {.oper_enum = LESS,     .oper_str = "jb",   .oper_byte = JB_BYTE},
    {.oper_enum = MORE_E,   .oper_str = "jae",  .oper_byte = JAE_BYTE},
    {.oper_enum = LESS_E,   .oper_str = "jbe",  .oper_byte = JBE_BYTE},
    {.oper_enum = EQUAL,    .oper_str = "je",   .oper_byte = JE_BYTE},
    {.oper_enum = N_EQUAL,  .oper_str = "jne",  .oper_byte = JNE_BYTE},
};

const static WhatReg RegArray[] =
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




#endif
