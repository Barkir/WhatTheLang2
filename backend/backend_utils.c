#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "what_lang/tree.h"
#include "what_lang/nametable.h"

#include "what_lang/list.h"
#include "what_lang/htable.h"

#include "what_lang/parser.h"

#include "what_lang/backend.h"
#include "what_lang/backend_utils.h"

const char * Adr2Reg(int adr, int xtnd)
{
    if (xtnd)
    {
        switch(adr)
        {
            case WHAT_REG_R8:   return "r8";
            case WHAT_REG_R9:   return "r9";
            case WHAT_REG_R10:  return "r10";
            case WHAT_REG_R11:  return "r11";
            case WHAT_REG_R12:  return "r12";
            case WHAT_REG_R13:  return "r13";
            case WHAT_REG_R14:  return "r14";
            case WHAT_REG_R15:  return "r15";
        }
    }

    switch(adr)
    {
            case 0:     return "ebx";
            case 1:     return "ecx";
            case 2:     return "edx";
            case 3:     return "esi";
            case 4:     return "edi";
    }
}

const enum Registers Adr2EnumReg(int adr)
{
    switch(adr)
    {
        case 0:     return WHAT_REG_EBX;
        case 1:     return WHAT_REG_ECX;
        case 2:     return WHAT_REG_EDX;
        case 3:     return WHAT_REG_ESI;
        case 4:     return WHAT_REG_EDI;
    }

}

const char * Reg2Str(int reg, int xtnd)
{
    if (xtnd)
    {
        switch(reg)
        {
            case WHAT_REG_R8:   return "r8";
            case WHAT_REG_R9:   return "r9";
            case WHAT_REG_R10:  return "r10";
            case WHAT_REG_R11:  return "r11";
            case WHAT_REG_R12:  return "r12";
            case WHAT_REG_R13:  return "r13";
            case WHAT_REG_R14:  return "r14";
            case WHAT_REG_R15:  return "r15";
        }
    }

    switch(reg)
    {
        case WHAT_REG_EAX:  return "rax";
        case WHAT_REG_EBX:  return "rbx";
        case WHAT_REG_ECX:  return "rcx";
        case WHAT_REG_EDX:  return "rdx";
        case WHAT_REG_ESI:  return "rsi";
        case WHAT_REG_EDI:  return "rdi";
        case WHAT_REG_EBP:  return "rbp";
        case WHAT_REG_ESP:  return "rsp";
    }
}

const char * GetVarName(Node * root)
{
    switch((int) NodeValue(root))
    {
        case 'a': return "AX";
        case 'b': return "BX";
        case 'c': return "CX";
        case 'x': return "DX";
        case 'y': return "FX";
        default : return "RX";
    }
    return "RX";
}

int GetVarAdr(Node * root, Name * names)
{
    for(int i = 0; i < 1024; i++)
    {
        if (!(names[i].name)) return -1;
        if (!strcmp(NodeName(root), names[i].name) && names[i].type == VAR) return names[i].address;
    }

    return -1;
}

Name * GetFuncAdr(Node * root, Name * names)
{
    for(int i = 0; i < 1024; i++)
    {
        if (!(names[i].name)) return NULL;
        if (!strcmp(NodeName(root), names[i].name) && names[i].type == FUNC_NAME) return &(names[i]);
    }

    return NULL;
}

int _count_param(Node * root)
{
    if (NodeType(root) == VAR) return 0;
    int i = 0;
    Node * dummy = root;
    while (dummy->left)
    {
        i++;
        dummy = dummy->left;
    }
    return i;
}

int _var_table(Node * root, Name * names, const char * func_name)
{
    if (NodeType(root) == VAR)
    {
        int is_new = 1;
        for (int i = 0; names[i].name; i++)
        {
            if (!strcmp(names[i].name, NodeName(root)))
            {

                is_new = 0;
                break;
            }
        }
        if (is_new)
        {
            names[ADR_COUNT].name = NodeName(root);
            names[ADR_COUNT].address = ADR_COUNT?(names[ADR_COUNT - 1].address_end + 1) : ADR_COUNT;
            names[ADR_COUNT].param = _count_param(root);
            names[ADR_COUNT].address_end = names[ADR_COUNT].address + names[ADR_COUNT].param;
            names[ADR_COUNT].type = VAR;
            names[ADR_COUNT].func_name = func_name;
            ADR_COUNT++;
        }
    }
    if (root->left)
        {
            if (NodeType(root) == FUNC_NAME) func_name = NodeName(root);
            _var_table(root->left, names, func_name);
        }
    if (root->right)
            {
            if (NodeType(root) == OPER && (int) NodeValue(root) == DEF) func_name = NodeName(root);
            _var_table(root->right, names, func_name);
        }
    return 0;
}

int _func_table(Node * root, Name * names)
{
    if (NodeType(root) == OPER && NodeValue(root) == DEF)
    {
        int is_new = 1;
        for (int i = 0; names[i].name; i++)
        {
            if (!strcmp(names[i].name, NodeName(root->left)))
            {
                is_new = 0;
                break;
            }
        }
        if (is_new)
        {
            names[ADR_COUNT].name = NodeName(root->left);
            names[ADR_COUNT].address = ADR_COUNT;
            names[ADR_COUNT].type = FUNC_NAME;
            names[ADR_COUNT].param = _count_param(root->left);
            names[ADR_COUNT].address_end = names[ADR_COUNT].address + names[ADR_COUNT].param;
            ADR_COUNT++;
        }
    }
    if (root->left) _func_table(root->left, names);
    if (root->right)_func_table(root->right, names);
    return 0;
}

Name * CreateVarTable(Node * root)
{
    Name * names = calloc(1024, sizeof(Name));
    _var_table(root, names, "");
    return names;
}

Name * CreateFuncTable(Node * root)
{
    Name * funcs = calloc(1024, sizeof(Name));
    _func_table(root, funcs);
    return funcs;
}

int _find_func_start(Name * names, const char * func_name)
{
    for (int i = 0; names[i].name; i++)
    {
        if (!strcmp(names[i].func_name, func_name)) return names[i].address;
    }
    return -1;

}

int _find_func_end(Name * names, const char * func_name)
{
    int end = -1;
        for (int i = 0; names[i].name; i++)
    {
        if (!strcmp(names[i].func_name, func_name)) end = end > names[i].address ? end : names[i].address;
    }

    return end;

}

