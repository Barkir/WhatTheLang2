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

const char * NASM_TOP = "%%include 'iolib/iolib.asm'    \n"
                        "section .text                  \n"
                        "global _start                  \n"
                        "_start:                        \n"
                        "mov r13, stack4calls           \n";

const char * NASM_BTM = "mov rax, 60                    \n"
                        "syscall                        \n"
                        "section .data                  \n"
                        "stack4calls times 128 * 8 db 0 \n"
                        "section .text                  \n";



#endif
