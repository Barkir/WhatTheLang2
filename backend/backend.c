#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <assert.h>

#include "what_lang/constants.h"
#include "what_lang/nametable.h"
#include "what_lang/list.h"             // List Structure
#include "what_lang/htable.h"           // Hash Table (includes list)
#include "what_lang/hashtable_errors.h"
#include "what_lang/tree.h"             // Binary Tree Structure
#include "what_lang/parser.h"           // Included for operations enum (bad)
#include "what_lang/errors.h"           // Errors and loggers
#include "what_lang/backend.h"          // Backend Header
#include "what_lang/emitters.h"         // Emitters Functions
#include "what_lang/backend_utils.h"    // Backend Utils Header
#include "what_lang/nasm2elf.h"

int CreateBin(Tree * tree, const char * filename_asm, const char * filename_bin, enum RunModes mode)
{
    assert(tree);
    assert(filename_asm);
    assert(filename_bin);

    FILE * fp = fopen(filename_asm, "wb");
    if (!fp)
    {
        perror("Error opening file fp");
        return WHAT_FILEOPEN_ERROR;
    }

    FILE * bin = fopen(filename_bin, "wb");
    if (!bin)
    {
        fclose(fp);
        perror("Error opening file what.out");
        return WHAT_FILEOPEN_ERROR;
    }

    Htable * names = CreateNameTable(tree->root);
    if (!names)
    {
        fclose(fp);
        fclose(bin);
        return WHAT_MEMALLOC_ERROR;
    }

    char * buf = calloc(BUF_DEF_SIZE, 1);
    if (!buf)
    {
        fclose(fp);
        fclose(bin);
        free(names);
        return WHAT_MEMALLOC_ERROR;
    }

    char * buf_ptr = buf;
    GenerateElfHeader(&buf);

    Htable * tab = NULL;
    if (HtableInit(&tab, HTABLE_BINS) != WHAT_SUCCESS)
    {
        free(buf_ptr);
        free(names);
        fclose(fp);
        fclose(bin);
    }

    BinCtx ctx =
    {
        .file = fp,
        .names = names,
        .func_name = GLOBAL_FUNC_NAME,
        .buf_ptr=buf_ptr,
        .buf = buf
    };

    EmitTop(&ctx);
    _create_bin(&ctx, &tab, tree->root);
    EmitBtm(&ctx);

    _def_bin(&ctx, &tab, tree->root);

    assert(WriteIOLib(&ctx) == WHAT_SUCCESS);

    if (fwrite(buf_ptr, sizeof(char), BUF_DEF_SIZE, bin) != BUF_DEF_SIZE)
        return WHAT_FILEWRITE_ERROR;

    if (mode == WHAT_DEBUG_MODE)
    {
        TreeDump  (tree,  TREEDUMP_FNAME);
        HtableDump(tab,   HTABLE_LOCALS_FNAME);
        HtableDump(names, HTABLE_NAMES_FNAME);
    }

    free(tab);
    free(buf_ptr);
    free(names);
    fclose(fp);
    fclose(bin);

    return WHAT_SUCCESS;
}

Htable * CreateNameTable(Node * root)
{
    assert(root);

    Htable * name_tab = NULL;
    HtableInit(&name_tab, HTABLE_BINS);

    NameTableCtx nmt_ctx = {.tab = &name_tab, .func_name = GLOBAL_FUNC_NAME, .stack_offset = 0};

    _create_name_table(root, &name_tab, &nmt_ctx);
    return name_tab;
}

/*
##########################################################################################################
##########################################################################################################
##########################################################################################################
*/

void BinCmpOper(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PrintNasmNode(root, ctx);

    int nodeVal = (int) NodeValue(root);

    if (root->left) _create_bin(ctx, tab, root->left );
    if (root->right)_create_bin(ctx, tab, root->right);

    EmitComparsion(ctx, tab, nodeVal);
}

void BinArithOper(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    int nodeVal = (int) NodeValue(root);

    if (nodeVal == '=')
    {
        PARSER_LOG("BinArithOper in '=' condition");
        if (root->right) _create_bin(ctx, tab, root->right);

        PrintNasmNode(root, ctx);

        assert(root->left);
        assert(GetVarFuncName(root->left, ctx));

        if (!strcmp(GetVarFuncName(root->left, ctx), GLOBAL_FUNC_NAME))
        {
            EmitPopReg      (ctx, Offset2EnumReg(GetVarOffset(root->left, ctx)));
            EmitPushXtendReg(ctx, WHAT_REG_R12);
            EmitAddRegVal   (ctx, WHAT_REG_R12, GetVarOffset(root->left, ctx) * sizeof(int64_t), WHAT_XTEND_VAL);
            EmitMovRegReg   (ctx, WHAT_REG_R12, Offset2EnumReg(GetVarOffset(root->left, ctx)), WHAT_XTEND_REG, WHAT_MEM1);
            EmitPopXtendReg (ctx, WHAT_REG_R12);
        }
        else
        {
            EmitPopReg(ctx, Offset2EnumReg(GetVarParam(root->left, ctx)));
        }

        return;
    }

    if (root->left) _create_bin(ctx, tab, root->left );
    if (root->right)_create_bin(ctx, tab, root->right);


    PrintNasmNode(root, ctx);
    if (nodeVal == '+')
    {
        EmitPopXtendReg (ctx, WHAT_REG_R14);
        EmitPopXtendReg (ctx, WHAT_REG_R15);
        EmitAddRegReg   (ctx, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);
        EmitPushXtendReg(ctx, WHAT_REG_R14);
        return;
    }

    else if (nodeVal == '-')
    {
        EmitPopXtendReg (ctx, WHAT_REG_R14);
        EmitPopXtendReg (ctx, WHAT_REG_R15);
        EmitSubRegReg   (ctx, WHAT_REG_R15, WHAT_REG_R14, WHAT_XTEND_XTEND);
        EmitPushXtendReg(ctx, WHAT_REG_R15);
        return;
    }

    else if (nodeVal == '*')
    {

        EmitPopXtendReg(ctx, WHAT_REG_R14);
        EmitPopReg     (ctx, WHAT_REG_EAX);
        EmitMulXtendReg(ctx, WHAT_REG_R14);
        EmitPushReg    (ctx, WHAT_REG_EAX);
        return;
    }


    else if (nodeVal == '/')
    {

        EmitPopXtendReg(ctx, WHAT_REG_R14);
        EmitPopReg     (ctx, WHAT_REG_EAX);
        EmitDivXtendReg(ctx, WHAT_REG_R14);
        EmitPushReg    (ctx, WHAT_REG_EAX);
        return;
    }

}

int BinIf(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PARSER_LOG("PROCESSING IF");
    PrintNasmNode(root, ctx);

    int local_if  = IF_COUNT;
    ctx->if_count = IF_COUNT;
    IF_COUNT++;

    Name * locals_if = InitLocalName(LABEL_SIZE);                               // Structure where local label
    assert(locals_if);                                                          // name will contain

    if (root->left) BinIfBlock(ctx, tab, root->left);
    PARSER_LOG("PROCESSED IF BLOCK... if_count = %d", ctx->if_count);

    InsertOffsetToLabel(ctx, locals_if, tab, "IF", local_if);

    fprintf(ctx->file, "IF%d:\n", ctx->if_count);

    EmitPushImm32(ctx, 0);

    EmitCmpRegsBlock(ctx);                                                      // Comparing values, popping them to r14 and r15

    char * cond   = EmitCondJmp(ctx, NULL, "jne COND",   ctx->if_count, JNE_BYTE, 0); // >>>>>>>>>>>>>>>>>>>>>>> v
    char * if_end = EmitCondJmp(ctx, NULL, "jmp IF_END", ctx->if_count, JMP_BYTE, 0); // >>>>>>>>>>>>>>>> v      v
                                                                                //                        v      v
    InsertOffsetToPtr(ctx, cond, "COND", ctx->if_count); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< v      v
                                                                                //                               v
    ctx->if_count++;                                                            //                               v
    if (root->right) BinIfBlock(ctx, tab, root->right);                         //                               v
                                                                                //                               v
    InsertOffsetToPtr  (ctx, if_end,"IF_END", local_if); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< v
    InsertOffsetToLabel(ctx, locals_if, tab, "IF_END", local_if);
    return WHAT_SUCCESS;

}

int BinWhile(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PARSER_LOG("PROCESSING WHILE");
    PrintNasmNode(root, ctx);

    int local_while = WHILE_COUNT;
    ctx->while_count = WHILE_COUNT;
    WHILE_COUNT++;

    Name * locals_while = InitLocalName(LABEL_SIZE);                           // Structure where local label
    assert(locals_while);                                                      // will contain

    fprintf(ctx->file, "WHILE%d:\n", ctx->while_count);
    const char * while_ptr = ctx->buf - 5;                                     // -5 is here to include previous instructions

    if (root->left) BinWhileBlock(ctx, tab, root->left);
    PARSER_LOG("PROCESSED WHILE BLOCK");


    InsertOffsetToLabel(ctx, locals_while, tab, "WHILE_FALSE", local_while);
    fprintf(ctx->file, "WHILE_FALSE%d:\n", ctx->while_count);

    EmitPushImm32(ctx, 0);

    EmitCmpRegsBlock(ctx);                                                      // Comparing values, popping them to r14 and r15

    char * while_end_ptr = EmitCondJmp(ctx, NULL, "je WHILE_END", local_while, JE_BYTE, 0); // >>>>> v
                                                                                      //       v
    fprintf(ctx->file, "WHILE_TRUE%d:\n", ctx->while_count);                          //       v
    InsertOffsetToLabel(ctx, locals_while, tab, "WHILE_TRUE", local_while);           // >>>>> v >>> v
                                                                                      //       v     v
    if (root->right) BinWhileBlock(ctx, tab, root->right);                            //       v     v
                                                                                      //       v     v
    fprintf(ctx->file, "jmp WHILE%d\n", local_while); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< v <<< v
    EmitLongJmp(ctx, (int)((while_ptr) - ctx->buf));                                  //       v
                                                                                      //       v
    InsertOffsetToPtr(ctx, while_end_ptr, "WHILE_END", local_while); // <<<<<<<<<<<<<<<<<<<<<<<v
    return WHAT_SUCCESS;
}

int BinFuncExt(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(tab);
    assert(root);
    assert(ctx);

    PrintNasmNode(root, ctx);
    int nodeVal = (int) NodeValue(root);

    if (nodeVal == PRINT)
    {
        if (root->left) _create_bin(ctx, tab, root->left);
        EmitPrint(ctx);
    }

    else if (nodeVal == INPUT)
    {
        EmitInput(ctx);
    }

    return WHAT_SUCCESS;
}

/*
##########################################################################################################
##########################################################################################################
##########################################################################################################
*/

int _create_bin(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(ctx);
    assert(tab);
    if (!root) return WHAT_SUCCESS;

    if (NodeType(root) == NUM)
    {
        PrintNasmNode(root, ctx);
        PARSER_LOG("PUSHING IMM32");
        EmitPushImm32(ctx, (int) NodeValue(root));
    }
    else if (NodeType(root) == FUNC_INTER_CALL)
    {
        PrintNasmNode(root, ctx);
        PARSER_LOG("FUNC INTER_CALL");
        Name ** param_array = GetFuncNameArray(root, ctx);

        int param = 0;
        for (Node * left = root->left; left; left = left->left, param++)
        {
            if (NodeType(left) == NUM)
            {
                EmitNumParam(ctx, left, param_array, param);
            }
            else if (NodeType(left) == VAR)
            {
                assert(ctx->func_name);
                assert(NodeName(root));

                if (!strcmp(NodeName(root), ctx->func_name)) EmitPushReg(ctx, Offset2EnumReg(param));
                else                                         EmitVarParam(ctx, left, param);
            }

            PARSER_LOG("param = %d %s", param, param_array[param]->name);
            EmitPopReg(ctx, Offset2EnumReg(param_array[param]->param));
        }
        EmitCallDirect(ctx, root);
        ctx->func_name = NodeName(root);
    }
    else if (NodeType(root) == VAR)
    {
        assert(GetVarFuncName(root, ctx));
        PrintNasmNode(root, ctx);

        if (!strcmp(GetVarFuncName(root, ctx), GLOBAL_FUNC_NAME)) EmitVar(ctx, root);
        else EmitPushReg(ctx, Offset2EnumReg(GetVarParam(root, ctx)));
    }
    else if (NodeType(root) == OPER)
    {
        PrintNasmNode(root, ctx);

        int nodeVal = (int) NodeValue(root);

        if      (isCmpOper(nodeVal))    BinCmpOper  (ctx, tab, root);
        else if (isArithOper(nodeVal))  BinArithOper(ctx, tab, root);
        else if (nodeVal == IF)         BinIf       (ctx, tab, root);
        else if (nodeVal == WHILE)      BinWhile    (ctx, tab, root);
    }
    else if (NodeType(root) == FUNC_EXT) BinFuncExt(ctx, tab, root);
    else if ((int) NodeValue(root) == ';')
    {
        _create_bin(ctx, tab, root->left );
        _create_bin(ctx, tab, root->right);
    }

    return WHAT_SUCCESS;
}

int _def_bin(BinCtx * ctx, Htable ** tab, Node * root)
{
    assert(ctx);
    assert(tab);
    if (!root) return WHAT_SUCCESS;

    if (NodeType(root) == OPER && (int) NodeValue(root) == DEF)
    {
        PrintNasmNode(root, ctx);
        const char * func_start = ctx->buf;

        EmitFuncStackPush(ctx, root);

        _create_bin(ctx, tab, root->right);

        EmitFuncStackRet(ctx, root);


        char ** func_adr = GetFuncAdrArr(root->left, ctx);
        PARSER_LOG("adr_array = %p", func_adr)
        int cap = GetFuncAdrArrCap(root->left, ctx);
        for (int i = 0; i < cap; i++)
        {
            int adr = func_start - func_adr[i] - 4;
            PARSER_LOG("adr = %d, i = %d, func_adr[%d] = %p, cap = %d", adr, i, i, func_adr[i], cap);
            memcpy(func_adr[i], &adr, sizeof(int));
        }
    }
    if (root->left)  _def_bin(ctx, tab, root->left );
    if (root->right) _def_bin(ctx, tab, root->right);
    return 1;
}

int _create_name_table(Node * root, Htable ** name_tab, NameTableCtx * ctx)
{
    assert(root);
    assert(name_tab);
    assert(ctx);

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
    }
    else if (NodeType(root) == FUNC_INTER_DEF)
    {
        PARSER_LOG("Got FUNC_INTER_DEF with name %s (stack_offset = %d)", NodeName(root), ctx->stack_offset);
        Name name = {.name = NodeName(root), .type = FUNC_INTER_DEF, .func_name = ctx->func_name};
        ctx->func_name = NodeName(root);

        name.name_array = calloc(DEFAULT_NAME_ARRAY_SIZE, sizeof(Name*));
        if (!name.name_array) return WHAT_MEMALLOC_ERROR;
        InitFuncParam(root->left, name_tab, &name, &ctx);

        HtableNameInsert(name_tab, &name);

        if (root->right) _create_name_table(root->right, name_tab, ctx);
        return WHAT_SUCCESS;
    }


    if (root->left)     _create_name_table(root->left,  name_tab, ctx);
    if (root->right)    _create_name_table(root->right, name_tab, ctx);
    return WHAT_SUCCESS;
}








