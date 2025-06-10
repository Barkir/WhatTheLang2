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
#include "what_lang/hashtable_errors.h"

int InitFuncParam(Node * root, Htable ** name_tab, NameTableCtx ** ctx)
{
    Name param_name = {.name = NodeName(root), .type = VAR, .func_name = (*ctx)->func_name, .stack_offset = ((*ctx)->stack_offset++)};
    PARSER_LOG("Param name = %s, function_name = %s, offset = %d", param_name.name, param_name.func_name, param_name.stack_offset);

    HtableNameInsert(name_tab, &param_name);
    if (root->left) InitFuncParam(root->left, name_tab, ctx);

    return WHAT_SUCCESS;
}

int _create_name_table(Node * root, Htable ** name_tab, NameTableCtx * ctx)
{
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
        PARSER_LOG("Inserted name in hash_table");
    }
    else if (NodeType(root) == FUNC_INTER_DEF)
    {
        PARSER_LOG("Got FUNC_INTER_DEF with name %s (stack_offset = %d)", NodeName(root), ctx->stack_offset);
        Name name = {.name = NodeName(root), .type = FUNC_INTER_DEF, .func_name = ctx->func_name};
        ctx->func_name = NodeName(root);

        name.name_array = calloc(DEFAULT_NAME_ARRAY_SIZE, sizeof(Name*));
        if (!name.name_array) return WHAT_MEMALLOC_ERROR;

        PARSER_LOG("Intializing parameters of function %s", NodeName(root));
        InitFuncParam(root->left, name_tab, &ctx);

        HtableNameInsert(name_tab, &name);
        PARSER_LOG("Inserted definition of function %s in hash table", NodeName(root));

        if (root->right) _create_name_table(root->right, name_tab, ctx);
        return WHAT_SUCCESS;
    }


    if (root->left)     _create_name_table(root->left,  name_tab, ctx);
    if (root->right)    _create_name_table(root->right, name_tab, ctx);
    return WHAT_SUCCESS;
}

Htable * CreateNameTable(Node * root)
{
    PARSER_LOG("Creating Name Table");
    Htable * name_tab = NULL;
    HtableInit(&name_tab, HTABLE_BINS);
    PARSER_LOG("Created Hash Table");

    NameTableCtx nmt_ctx = {.tab = &name_tab, .func_name = GLOBAL_FUNC_NAME, .stack_offset = 0};

    _create_name_table(root, &name_tab, &nmt_ctx);
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

void BinCmpOper(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    int nodeVal = (int) NodeValue(root);
    _create_bin(buf, tab, root->left,  ctx);
    _create_bin(buf, tab, root->right, ctx);
    EMIT_COMPARSION(buf, tab, nodeVal, ctx);
    return;

}

void BinArithOper(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    int nodeVal = (int) NodeValue(root);
    PARSER_LOG("calling BinArithOper with NodeValue %c %d", nodeVal, nodeVal);

    if (nodeVal == '=')
    {
        PARSER_LOG("BinArithOper in '=' condition");
        _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, *if_count, *while_count);
        POPREG(buf, file, Offset2EnumReg(GetVarOffset(root->left, names)));
        return;
    }


    _create_bin(buf, tab, root->left,  ctx);
    _create_bin(buf, tab, root->right, ctx);

    if (nodeVal == '+')
    {

        POP_XTEND_REG (buf, file, WHAT_REG_R14);
        POP_XTEND_REG (buf, file, WHAT_REG_R15);
        ADD_REG_REG   (buf, file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);
        PUSH_XTEND_REG(buf, file, WHAT_REG_R14);
        return;
    }

    else if (nodeVal == '-')
    {
        POP_XTEND_REG (buf, file, WHAT_REG_R14);
        POP_XTEND_REG (buf, file, WHAT_REG_R15);
        SUB_REG_REG   (buf, file, WHAT_REG_R15, WHAT_REG_R14, WHAT_XTEND_XTEND);
        PUSH_XTEND_REG(buf, file, WHAT_REG_R15);
        return;
    }

    else if (nodeVal == '*')
    {

        POP_XTEND_REG(buf, file, WHAT_REG_R14);
        POPREG       (buf, file, WHAT_REG_EAX);
        MUL_XTEND_REG(buf, file, WHAT_REG_R14);
        PUSHREG      (buf, file, WHAT_REG_EAX);
        return;
    }


    else if (nodeVal == '/')
    {

        POP_XTEND_REG(buf, file, WHAT_REG_R14);
        POPREG       (buf, file, WHAT_REG_EAX);
        DIV_XTEND_REG(buf, file, WHAT_REG_R14);
        PUSHREG      (buf, file, WHAT_REG_EAX);
        return;
    }

}

int BinIf(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
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

int BinWhile(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
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

int BinFunc(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    PARSER_LOG("CALLED BIN_FUNC");
    int nodeVal = (int) NodeValue(root);


    if (nodeVal == PRINT)
    {
        _create_bin(buf, tab, ctx, root->left);
        EMIT_PRINT(buf, file);
    }

    else if (nodeVal == INPUT)
    {
        EMIT_INPUT(buf, file);
    }

    return WHAT_SUCCESS;
}


const char * Offset2StrReg(int adr, int xtnd)
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

const enum Registers Offset2EnumReg(int adr)
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

int GetVarOffset(Node * root, Htable * names)
{
    Name root_name = {.name = NodeName(root), .type = NodeType(root), .func_name = }
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
}


