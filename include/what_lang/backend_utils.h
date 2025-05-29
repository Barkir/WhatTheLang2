#ifndef BACKEND_UTILS_H
#define BACKEND_UTILS_H

const enum Registers Adr2EnumReg(int adr);
const char * Adr2Reg(int adr, int xtnd);
const char * Reg2Str(int reg, int xtnd);

Name * CreateVarTable(Node * root);
Name * CreateFuncTable(Node * root);
const char * GetVarName(Node * root);
int GetVarAdr(Node * root, Name * names);
Name * GetFuncAdr(Node * root, Name * names);

int _count_param(Node * root);
int _var_table(Node * root, Name * names, const char * func_name);
int _func_table(Node * root, Name * names);
int _find_func_start(Name * names, const char * func_name);
int _find_func_end(Name * names, const char * func_name);


int             DefineFuncTable(Name ** func, Name ** names);

// Compare functions (used in BinCmpOper, etc.)
int             isCmpOper(enum operations oper_enum);
int             isArithOper(enum operations oper_enum);
const char *    CmpStr(enum operations oper_enum);
char            CmpByte(enum operations oper_enum);


//  Functions for generating binary code from if, while or opers (yep, i know there is bad parameters, fixing it soon...)
void            BinCmpOper  (char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int * if_count, int * while_count);
void            BinArithOper(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int * if_count, int * while_count);
int             BinWhile    (char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int   if_count, int   while_count);
int             BinIf       (char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int   if_count, int   while_count);
int             BinFunc     (char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int   if_count, int   while_count);

static const char *     NASM_TOP =  "%%include 'iolib/iolib.asm'    \n"
                                    "section .text                  \n"
                                    "global _start                  \n"
                                    "_start:                        \n"
                                    "mov r13, stack4calls           \n";

static const char *     NASM_BTM =  "mov rax, 60                    \n"
                                    "syscall                        \n"
                                    "section .data                  \n"
                                    "stack4calls times 128 * 8 db 0 \n"
                                    "section .text                  \n";

typedef struct _cmp_oper
{
    enum operations oper_enum;
    const char * oper_str;
    char oper_byte;

} CmpOper;

#define OPER_ARRAY_SIZE 6

const static CmpOper OperArray[OPER_ARRAY_SIZE] =
{
    {.oper_byte = MORE,     .oper_str = "ja",   .oper_byte = JA_BYTE},
    {.oper_byte = LESS,     .oper_str = "jb",   .oper_byte = JB_BYTE},
    {.oper_byte = MORE_E,   .oper_str = "jae",  .oper_byte = JAE_BYTE},
    {.oper_byte = LESS_E,   .oper_str = "jbe",  .oper_byte = JBE_BYTE},
    {.oper_byte = EQUAL,    .oper_str = "je",   .oper_byte = JE_BYTE},
    {.oper_byte = N_EQUAL,  .oper_str = "jne",  .oper_byte = JNE_BYTE}
};




#endif
