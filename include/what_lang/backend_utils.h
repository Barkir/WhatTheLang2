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

#endif
