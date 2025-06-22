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

int NameArrayAddElem(Name * name, Name * elem)
{
    PARSER_LOG("Adding param to function with name %s", name->name);
    Name ** array = name->name_array;
    PARSER_LOG("name_array = %p", array);
    for (int i = 0; i < DEFAULT_NAME_ARRAY_SIZE; i++)
    {
        if (!array[i])
        {
            array[i] = elem;
            array[i]->param = i;
            PARSER_LOG("Added name %s to place %p", array[i]->name, array);
            return WHAT_SUCCESS;
        }
    }
    return WHAT_SUCCESS;
}

int InitFuncParam(Node * root, Htable ** name_tab, Name * function, NameTableCtx ** ctx)
{
    Name param_name = {.name = NodeName(root), .type = VAR, .func_name = (*ctx)->func_name, .stack_offset = ((*ctx)->stack_offset++)};
    PARSER_LOG("Param name = %s, function_name = %s, offset = %d", param_name.name, param_name.func_name, param_name.stack_offset);

    HtableNameInsert(name_tab, &param_name);
    NameArrayAddElem(function, HtableNameFind(*name_tab, &param_name));
    if (root->left) InitFuncParam(root->left, name_tab, function, ctx);

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
        PARSER_LOG("created name_array %p", name.name_array);

        PARSER_LOG("Intializing parameters of function %s", NodeName(root));
        InitFuncParam(root->left, name_tab, &name, &ctx);

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
    PARSER_LOG("Calling BinCmpOper...");
    int nodeVal = (int) NodeValue(root);
    _create_bin(buf, tab, root->left,  ctx);
    _create_bin(buf, tab, root->right, ctx);
    PARSER_LOG("Calling EMIT_COMPARSION, if_count = %d, while_count = %d", ctx->if_count, ctx->while_count);
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
        _create_bin(buf, tab, root->right, ctx);


        if (!strcmp(GetVarFuncName(root->left, ctx), GLOBAL_FUNC_NAME))
        {
            POPREG(buf, Offset2EnumReg(GetVarOffset(root->left, ctx)), ctx);
            PUSH_XTEND_REG(buf, WHAT_REG_R12, ctx);
            ADD_REG_VAL(buf, WHAT_REG_R12, GetVarOffset(root->left, ctx) * 8, WHAT_XTEND_VAL, ctx);
            MOV_REG_REG(buf, WHAT_REG_R12, Offset2EnumReg(GetVarOffset(root->left, ctx)), WHAT_XTEND_REG, WHAT_MEM1, ctx);
            POP_XTEND_REG(buf, WHAT_REG_R12, ctx);
        }
        else
        {
            POPREG(buf, Offset2EnumReg(GetVarParam(root->left, ctx)), ctx);
        }

        return;
    }

    _create_bin(buf, tab, root->left,  ctx);
    _create_bin(buf, tab, root->right, ctx);

    if (nodeVal == '+')
    {

        POP_XTEND_REG (buf, WHAT_REG_R14, ctx);
        POP_XTEND_REG (buf, WHAT_REG_R15, ctx);
        ADD_REG_REG   (buf, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND, ctx);
        PUSH_XTEND_REG(buf, WHAT_REG_R14, ctx);
        return;
    }

    else if (nodeVal == '-')
    {
        POP_XTEND_REG (buf, WHAT_REG_R14, ctx);
        POP_XTEND_REG (buf, WHAT_REG_R15, ctx);
        SUB_REG_REG   (buf, WHAT_REG_R15, WHAT_REG_R14, WHAT_XTEND_XTEND, ctx);
        PUSH_XTEND_REG(buf, WHAT_REG_R15, ctx);
        return;
    }

    else if (nodeVal == '*')
    {

        POP_XTEND_REG(buf, WHAT_REG_R14, ctx);
        POPREG       (buf, WHAT_REG_EAX, ctx);
        MUL_XTEND_REG(buf, WHAT_REG_R14, ctx);
        PUSHREG      (buf, WHAT_REG_EAX, ctx);
        return;
    }


    else if (nodeVal == '/')
    {

        POP_XTEND_REG(buf, WHAT_REG_R14, ctx);
        POPREG       (buf, WHAT_REG_EAX, ctx);
        DIV_XTEND_REG(buf, WHAT_REG_R14, ctx);
        PUSHREG      (buf, WHAT_REG_EAX, ctx);
        return;
    }

}

int BinIf(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    PARSER_LOG("PROCESSING IF");
    int local_if  = IF_COUNT;
    ctx->if_count = IF_COUNT;
    IF_COUNT++;

    Name locals_if = {};
    locals_if.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));
    if (!locals_if.local_func_name) return WHAT_MEMALLOC_ERROR;

    ctx->if_cond    = 1;
    ctx->while_cond = 0;

    _create_bin(buf, tab, root->left, ctx);
    PARSER_LOG("PROCESSED IF BLOCK... if_count = %d", ctx->if_count);

    sprintf(locals_if.local_func_name, "IF%d", local_if);
    Name * label = HtableLabelFind(*tab, &locals_if);
    if (label) *label->offset = (int8_t) (*buf - (label->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    fprintf(ctx->file, "IF%d:\n", ctx->if_count);

    PUSHIMM32(buf, 0, ctx);

    POP_XTEND_REG   (buf, WHAT_REG_R15, ctx);
    POP_XTEND_REG   (buf, WHAT_REG_R14, ctx);
    PUSH_XTEND_REG  (buf, WHAT_REG_R14, ctx);
    PUSH_XTEND_REG  (buf, WHAT_REG_R15, ctx);

    CMP_REG_REG     (buf, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND, ctx);

    fprintf(ctx->file, "jne COND%d\n", ctx->if_count);
    EMIT_JMP(buf, JNE_BYTE, 0);
    char * cond = *buf - 1;

    fprintf(ctx->file, "jmp IF_END%d\n", ctx->if_count);
    EMIT_JMP(buf, JMP_BYTE, 0);
    char * if_end = *buf - 1;

    fprintf(ctx->file, "COND%d:\n", ctx->if_count);
    *cond = *buf - (cond + 1);

    ctx->if_count++;
    ctx->if_cond = 1;
    ctx->while_cond = 0;

    _create_bin(buf, tab, root->right, ctx);

    fprintf(ctx->file, "IF_END%d:\n", local_if);
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
    ctx->while_count = WHILE_COUNT;
    WHILE_COUNT++;

    Name locals_while = {};
    locals_while.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));
    if (!locals_while.local_func_name) return WHAT_MEMALLOC_ERROR;

    fprintf(ctx->file, "WHILE%d:\n", ctx->while_count);
    const char * while_ptr = *buf;
    PARSER_LOG("while_ptr = %p", while_ptr);

    ctx->if_cond = 0;
    ctx->while_cond = 1;
    _create_bin(buf, tab, root->left, ctx);
    PARSER_LOG("PROCESSED WHILE BLOCK");

    sprintf(locals_while.local_func_name, "WHILE_FALSE%d", local_while);
    Name * label_while = HtableLabelFind(*tab, &locals_while);
    if (label_while) *label_while->offset = (int8_t) (*buf - (label_while->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    fprintf(ctx->file, "WHILE_FALSE%d:\n", ctx->while_count);

    PUSHIMM32(buf, 0, ctx);

    POP_XTEND_REG   (buf, WHAT_REG_R15, ctx);
    POP_XTEND_REG   (buf, WHAT_REG_R14, ctx);
    PUSH_XTEND_REG  (buf, WHAT_REG_R14, ctx);
    PUSH_XTEND_REG  (buf, WHAT_REG_R15, ctx);

    CMP_REG_REG     (buf, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND, ctx);

    fprintf(ctx->file, "je WHILE_END%d\n", ctx->while_count);
    EMIT_JMP(buf,JE_BYTE, 0);
    char * while_end_ptr = *buf - 1;

    fprintf(ctx->file, "WHILE_TRUE%d:\n", ctx->while_count);

    sprintf(locals_while.local_func_name, "WHILE_TRUE%d", local_while);
    label_while = HtableLabelFind(*tab, &locals_while);
    if (label_while) *label_while->offset = (int8_t) (*buf - (label_while->offset + 1));
    else return WHAT_NOLABEL_ERROR;


    ctx->if_cond = 0;
    ctx->while_cond = 1;
    _create_bin(buf, tab, root->right, ctx);

    fprintf(ctx->file, "jmp WHILE%d\n", local_while);
    EMIT_LONG_JMP(buf, (int)((while_ptr - 5) - *buf));

    fprintf(ctx->file, "WHILE_END%d:\n", local_while);

    *while_end_ptr = *buf - (while_end_ptr + 1);

    fprintf(ctx->file, ";---------------------------\n\n");
    PARSER_LOG("%x %x %x %x %x %x [%x] %x %x %x %x", *(while_ptr - 6), *(while_ptr - 5), *(while_ptr - 4), *(while_ptr - 3), *(while_ptr - 2), *(while_ptr - 1) , *while_ptr, *(while_ptr + 1), *(while_ptr + 2), *(while_ptr + 3), *(while_ptr + 4));
    PARSER_LOG("while_ptr = %p", while_ptr);

    return WHAT_SUCCESS;
}

int BinFuncExt(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    PARSER_LOG("CALLED BIN_FUNC");
    int nodeVal = (int) NodeValue(root);


    if (nodeVal == PRINT)
    {
        _create_bin(buf, tab, root->left, ctx);
        EMIT_PRINT(buf, ctx);
    }

    else if (nodeVal == INPUT)
    {
        EMIT_INPUT(buf, ctx);
    }

    return WHAT_SUCCESS;
}

int AddFuncAdr(char ** buf, Node * root, BinCtx * ctx)
{
    Name func = {.name = NodeName(root), .type = FUNC_INTER_DEF};
    Name * found_func = HtableNameFind(ctx->names, &func);
    for (int i = 0; i < 32; i++)
    {
        if (!found_func->adr_array[i])
        {
            found_func->adr_array[i] = *buf;
            PARSER_LOG("adr_array[i] = %p", found_func->adr_array[i]);
            return WHAT_SUCCESS;
        }
    }

    return WHAT_NOTFOUND_ERROR;
}

const char * Offset2StrReg(int adr, int xtnd)
{
    PARSER_LOG("Calling Offset2StrReg");
    if (xtnd)
    {
        for (int i = 0; i < REG_ARRAY_SIZE; i++)
        {
            if (RegArray[i].reg_xtnd == adr) return RegArray[i].reg_str;
        }
    }

    switch(adr % DEFAULT_REG_NUMBER)
    {
            case 0:     return "rbx";
            case 1:     return "rcx";
            case 2:     return "rdx";
            case 3:     return "rsi";
            case 4:     return "rdi";
    }

    return NULL;
}

const enum Registers Offset2EnumReg(int adr)
{
    switch(adr % DEFAULT_REG_NUMBER)
    {
        case 0:     return WHAT_REG_EBX;
        case 1:     return WHAT_REG_ECX;
        case 2:     return WHAT_REG_EDX;
        case 3:     return WHAT_REG_ESI;
        case 4:     return WHAT_REG_EDI;
        default:    return WHAT_REG_UNK;
    }

    return WHAT_REG_UNK;

}

const char * GetVarFuncName(Node * root, BinCtx * ctx)
{
    Name root_name = {.name = NodeName(root), .type  = NodeType(root)};
    return HtableNameFind(ctx->names, &root_name)->func_name;
}

char ** GetFuncAdrArr(Node * root, BinCtx * ctx)
{
    Name root_name = {.name = NodeName(root), .type = FUNC_INTER_DEF};
    return HtableNameFind(ctx->names, &root_name)->adr_array;
}

int GetVarOffset(Node * root, BinCtx * ctx)
{
    Name root_name = {.name = NodeName(root), .type = NodeType(root)};
    return HtableNameFind(ctx->names, &root_name)->stack_offset;
}

int GetVarParam(Node * root, BinCtx * ctx)
{
    Name root_name = {.name = NodeName(root), .type = NodeType(root)};
    return HtableNameFind(ctx->names, &root_name)->param;
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

    return NULL;
}

Name ** GetFuncNameArray(Node * root, BinCtx * ctx)
{
    Name func = {.name = NodeName(root), .type=FUNC_INTER_DEF};
    return HtableNameFind(ctx->names, &func)->name_array;
}


