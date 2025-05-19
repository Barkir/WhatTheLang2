#ifndef BACKEND_H
#define BACKEND_H


Name * CreateVarTable(Node * root);
Name  * CreateFuncTable(Node * root);
int _var_table(Node * root, Name * names, const char * func_name);
int _func_table(Node * root, Name * names);
int GetVarAdr(Node * root, Name * names);
const char * GetVarName(Node * root);


const enum Registers Adr2EnumReg(int adr);
const char * Adr2Reg(int adr);

int _create_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count);
int _create_bin(char ** buf, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count);

static int IF_COUNT = 0;
static int WHILE_COUNT = 0;
static int ADR_COUNT = 0;


enum Registers
{
    WHAT_REG_EAX = 0x00,
    WHAT_REG_ECX = 0x01,
    WHAT_REG_EDX = 0x02,
    WHAT_REG_EBX = 0x03,
    WHAT_REG_ESP = 0x04,
    WHAT_REG_EBP = 0x05,
    WHAT_REG_ESI = 0x06,
    WHAT_REG_EDI = 0x07,
};

enum AdditionalRegisters
{
    WHAT_REG_R8  = 0x00,
    WHAT_REG_R9  = 0x01,
    WHAT_REG_R10 = 0x02,
    WHAT_REG_R11 = 0x03,
    WHAT_REG_R12 = 0x04,
    WHAT_REG_R13 = 0x05,
    WHAT_REG_R14 = 0x06,
    WHAT_REG_R15 = 0x07,
};

#endif
