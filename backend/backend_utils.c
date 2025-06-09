#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "what_lang/tree.h"
#include "what_lang/nametable.h"

#include "what_lang/list.h"
#include "what_lang/htable.h"

#include "what_lang/parser.h"

#include "what_lang/backend.h"
#include "what_lang/emit_constants.h"
#include "what_lang/backend_utils.h"
#include "what_lang/emitters.h"
#include "what_lang/errors.h"

const char * Adr2Reg(int adr, int xtnd)
{
    if (xtnd)
    {
        for (int i = 0; i < REG_ARRAY_SIZE; i++)
        {
            if (RegArray[i].reg_xtnd == adr) return RegArray[i].reg_str;
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
        for (int i = 0; i < REG_ARRAY_SIZE; i++)
            if (RegArray[i].reg_xtnd == reg) return RegArray[i].reg_str;
    }

    for (int i = 0; i < REG_ARRAY_SIZE; i++)
        if (RegArray[i].reg == reg) return RegArray[i].reg_str;
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
    PARSER_LOG("Getting Var Address");
    for(int i = 0; i < 1024; i++)
    {
        if (!(names[i].name)) return -1;
        if (!strcmp(NodeName(root), names[i].name) && names[i].type == VAR) return names[i].address;
    }

    return -1;
}

Name * GetFuncAdr(Node * root, Name * names)
{
    PARSER_LOG("Getting Func Address");
    for(int i = 0; i < 1024; i++)
    {
        PARSER_LOG("NodeName(root) = %s, names[i].name = %s", NodeName(root), names[i].name);
        if (!(names[i].name)) {PARSER_LOG("Returning NULL at i = %d", i); return NULL;}
        if (!strcmp(NodeName(root), names[i].name) && (names[i].type == FUNC_INTER_DEF || names[i].type == VAR)) return &(names[i]);
    }

    PARSER_LOG("Returning NULL");
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
    if (NodeType(root) == VAR && NodeType(root) == FUNC_INTER_CALL && NodeType(root) == FUNC_INTER_DEF)
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
            if (NodeType(root) == FUNC_INTER_DEF) func_name = NodeName(root);
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
            names[ADR_COUNT].type = FUNC_INTER_DEF;
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

int InitFuncParam(Node * root, Name ** name, Htable ** name_tab, NameTableCtx ** ctx)
{
    Name param_name = {.name = NodeName(root), .func_name = (*ctx)->func_name, .stack_offset = ((*ctx)->stack_offset++)};

    HtableNameInsert(name_tab, &param_name);
    if (root->left) InitFuncParam(root->left, name, name_tab, ctx);

    return WHAT_SUCCESS;
}

int _create_name_table(Node * root, Htable ** name_tab, NameTableCtx * ctx)
{
    if (NodeType(root) == VAR)
    {
        Name name = {.func_name = ctx->func_name, .name = NodeName(root), .type = VAR, .stack_offset = (ctx->stack_offset++)};
        HtableNameInsert(name_tab, &name);
    }

    else if (NodeType(root) == FUNC_EXT)
    {
        Name name = {.name = NodeName(root), .type = FUNC_EXT};
        HtableNameInsert(name_tab, &name);

    }

    else if (NodeType(root) == FUNC_INTER_CALL)
    {
        Name name = {.name = NodeName(root), .type = FUNC_INTER_CALL};
        HtableNameInsert(name_tab, &name);
    }

    else if (NodeType(root) == FUNC_INTER_DEF)
    {
        Name name = {.name = NodeName(root), .type = FUNC_INTER_DEF};
        name.name_array = calloc(DEFAULT_NAME_ARRAY_SIZE, sizeof(Name*));
        InitFuncParam(root->left, &name, name_tab, &ctx);

        HtableNameInsert(name_tab, &name);

        ctx->func_name = NodeName(root);
        _create_name_table(root->right, name_tab, ctx);
        return WHAT_SUCCESS;
    }


    if (root->left)     _create_name_table(root->left,  name_tab, ctx);
    if (root->right)    _create_name_table(root->right, name_tab, ctx);
    return WHAT_SUCCESS;
}

Htable * CreateNameTable(Node * root)
{
    Htable * name_tab = NULL;
    HtableInit(&name_tab, HTABLE_BINS);

    NameTableCtx nmt_ctx = {.tab = &name_tab, .func_name = GLOBAL_FUNC_NAME, .stack_offset = 0};

    _create_name_table(root, &nmt_ctx);
    return name_tab;
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

int DefineFuncTable(Name ** func, Name ** names)
{
    if (!*func)  return WHAT_NULLPOINTER_ERROR;
    if (!*names) return WHAT_NULLPOINTER_ERROR;

    for (int i = 0; (*names)[i].name; i++)
    {
        for(int j = 0; (*func)[j].name; j++)
        {
            if (!strcmp((*names)[i].name, (*func)[j].name))
            {
                PARSER_LOG("FOUND FUNC %s", (*func)[j].name);
                (*names)[i].name =  (*func)[j].name;
                (*names)[i].param = (*func)[j].param;
                (*names)[i].type =  (*func)[j].type;
                (*names)[i].address     = _find_func_start  (*names, (*names)[i].name);
                (*names)[i].address_end = _find_func_end    (*names, (*names)[i].name);
                break;
            }
        }
    }
}

char CmpByte(enum operations oper_enum)
{
    PARSER_LOG("%d in CmpByte", oper_enum);
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
    PARSER_LOG("calling isArithOper...");
    return oper_enum == '+' || oper_enum == '-' || oper_enum == '/' || oper_enum == '*' || oper_enum == '=' ;
}

void BinCmpOper(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int * if_count, int * while_count)
{
    int nodeVal = (int) NodeValue(root);
    _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, *if_count, *while_count);
    _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, *if_count, *while_count);
    EMIT_COMPARSION(buf, file, tab, CmpByte(nodeVal), CmpStr(nodeVal), if_count, while_count, if_cond, while_cond);
    return;

}

void BinArithOper(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int * if_count, int * while_count)
{
    int nodeVal = (int) NodeValue(root);
    PARSER_LOG("calling BinArithOper with NodeValue %c %d", nodeVal, nodeVal);

    if (nodeVal == '=')
    {
        PARSER_LOG("BinArithOper in '=' condition");
        _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, *if_count, *while_count);
        POPREG(buf, file, Adr2EnumReg(GetVarAdr(root->left, names)));
        return;
    }


    _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, *if_count, *while_count);
    _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, *if_count, *while_count);

    if (nodeVal == '+')
    {

        POP_XTEND_REG(buf, file, WHAT_REG_R14);
        POP_XTEND_REG(buf, file, WHAT_REG_R15);
        ADD_REG_REG(buf, file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);
        PUSH_XTEND_REG(buf, file, WHAT_REG_R14);
        return;
    }

    else if (nodeVal == '-')
    {
        POP_XTEND_REG(buf, file, WHAT_REG_R14);
        POP_XTEND_REG(buf, file, WHAT_REG_R15);
        SUB_REG_REG(buf, file, WHAT_REG_R15, WHAT_REG_R14, WHAT_XTEND_XTEND);
        PUSH_XTEND_REG(buf, file, WHAT_REG_R15);
        return;
    }

    else if (nodeVal == '*')
    {

        POP_XTEND_REG(buf, file, WHAT_REG_R14);
        POPREG(buf, file, WHAT_REG_EAX);
        MUL_XTEND_REG(buf, file, WHAT_REG_R14);
        PUSHREG(buf, file, WHAT_REG_EAX);
        return;
    }


    else if (nodeVal == '/')
    {

        POP_XTEND_REG(buf, file, WHAT_REG_R14);
        POPREG(buf, file, WHAT_REG_EAX);
        DIV_XTEND_REG(buf, file, WHAT_REG_R14);
        PUSHREG(buf, file, WHAT_REG_EAX);
        return;
    }

}

int BinIf(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    PARSER_LOG("PROCESSING IF");
    int local_if = IF_COUNT;
    if_count = IF_COUNT;
    IF_COUNT++;

    Name locals_if = {};
    locals_if.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));
    if (!locals_if.local_func_name) return WHAT_MEMALLOC_ERROR;

    _create_bin(buf, tab, names, root->left, file, 1, 0, if_count, while_count);
    PARSER_LOG("PROCESSED IF BLOCK...");

    sprintf(locals_if.local_func_name, "IF%d", local_if);
    Name * label = HtableLabelFind(*tab, &locals_if);
    if (label) *label->offset = (int8_t) (*buf - (label->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    fprintf(file, "IF%d:\n", if_count);

    PUSHIMM32(buf, file, 0);

    POP_XTEND_REG   (buf, file, WHAT_REG_R15);
    POP_XTEND_REG   (buf, file, WHAT_REG_R14);
    PUSH_XTEND_REG  (buf, file, WHAT_REG_R14);
    PUSH_XTEND_REG  (buf, file, WHAT_REG_R15);

    CMP_REG_REG     (buf, file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);

    fprintf(file, "jne COND%d\n", if_count);
    EMIT_JMP(buf, JNE_BYTE, 0);
    char * cond = *buf - 1;

    fprintf(file, "jmp IF_END%d\n", if_count);
    EMIT_JMP(buf, JMP_BYTE, 0);
    char * if_end = *buf - 1;

    fprintf(file, "COND%d:\n", if_count);
    *cond = *buf - (cond + 1);

    if_count++;
    _create_bin(buf, tab, names, root->right, file, 1, 0, if_count, while_count);

    fprintf(file, "IF_END%d:\n", local_if);
    *if_end = *buf - (if_end + 1);

    sprintf(locals_if.local_func_name, "IF_END%d", local_if);
    label = HtableLabelFind(*tab, &locals_if);
    if (label) *label->offset = (int8_t)(*buf - (label->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    return WHAT_SUCCESS;

}

int BinWhile(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    PARSER_LOG("PROCESSING WHILE");
    int local_while = WHILE_COUNT;
    while_count = WHILE_COUNT;
    WHILE_COUNT++;

    Name locals_while = {};
    locals_while.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));
    if (!locals_while.local_func_name) return WHAT_MEMALLOC_ERROR;

    fprintf(file, "WHILE%d:\n", while_count);
    char * while_ptr = *buf;

    _create_bin(buf, tab, names, root->left, file, 0, 1, if_count, while_count);
    PARSER_LOG("PROCESSED WHILE BLOCK");

    sprintf(locals_while.local_func_name, "WHILE_FALSE%d", local_while);
    Name * label_while = HtableLabelFind(*tab, &locals_while);
    if (label_while) *label_while->offset = (int8_t) (*buf - (label_while->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    fprintf(file, "WHILE_FALSE%d:\n", while_count);

    PUSHIMM32(buf, file, 0);

    POP_XTEND_REG   (buf, file, WHAT_REG_R15);
    POP_XTEND_REG   (buf, file, WHAT_REG_R14);
    PUSH_XTEND_REG  (buf, file, WHAT_REG_R14);
    PUSH_XTEND_REG  (buf, file, WHAT_REG_R15);

    CMP_REG_REG     (buf, file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);

    fprintf(file, "je WHILE_END%d\n", while_count);
    EMIT_JMP(buf,JE_BYTE, 0);
    char * while_end_ptr = *buf - 1;

    fprintf(file, "WHILE_TRUE%d:\n", while_count);

    sprintf(locals_while.local_func_name, "WHILE_TRUE%d", local_while);
    label_while = HtableLabelFind(*tab, &locals_while);
    if (label_while) *label_while->offset = (int8_t) (*buf - (label_while->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    _create_bin(buf, tab, names, root->right, file, 0, 1, if_count, while_count);

    fprintf(file, "jmp WHILE%d\n", local_while);
    EMIT_JMP(buf, JMP_BYTE, 0);
    (*buf)--;
    **buf = (while_ptr - 1) - *buf;
    (*buf)++;

    fprintf(file, "WHILE_END%d:\n", local_while);
    EMIT_JMP(buf, JMP_BYTE, 0);
    *while_end_ptr = *buf - (while_end_ptr + 1);

    fprintf(file, ";---------------------------\n\n");

    return WHAT_SUCCESS;
}

int BinFunc(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    PARSER_LOG("CALLED BIN_FUNC");
    int nodeVal = (int) NodeValue(root);


    if (nodeVal == PRINT)
    {
        _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
        EMIT_PRINT(buf, file);
    }

    else if (nodeVal == INPUT)
    {
        EMIT_INPUT(buf, file);
    }

    return WHAT_SUCCESS;
}


