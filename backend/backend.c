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
#include "what_lang/tree.h"             // Binary Tree Structure
#include "what_lang/parser.h"           // Included for operations enum (bad)
#include "what_lang/errors.h"           // Errors and loggers
#include "what_lang/backend.h"          // Backend Header
#include "what_lang/emitters.h"         // Emitters Functions
#include "what_lang/backend_utils.h"    // Backend Utils Header
#include "what_lang/nasm2elf.h"

int CreateBin(Tree * tree, const char * filename_asm, const char * filename_bin, enum RunModes mode)
{
    VERIFY_PTRS(tree, filename_asm, filename_bin);

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

/*
##########################################################################################################
##########################################################################################################
##########################################################################################################
*/

int _create_bin(BinCtx * ctx, Htable ** tab, Node * root)
{
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
                if (!strcmp(NodeName(root), ctx->func_name)) EmitPushReg(ctx, Offset2EnumReg(param));
                else                                         EmitVarParam(ctx, left, param);
            }

            PARSER_LOG("param = %d %s", param, param_array[param]->name);
            EmitPopReg(ctx, Offset2EnumReg(param_array[param]->param));
        }
        CallDirect(ctx, root);
        ctx->func_name = NodeName(root);
    }
    else if (NodeType(root) == VAR)
    {
        PrintNasmNode(root, ctx);
        if (!strcmp(GetVarFuncName(root, ctx), GLOBAL_FUNC_NAME)) EmitVar(ctx, root);
        else EmitPushReg(ctx, Offset2EnumReg(GetVarParam(root, ctx)));
    }
    else if (NodeType(root) == OPER)
    {
        PrintNasmNode(root, ctx);
        PARSER_LOG("PROCESSING OPER");

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
    PARSER_LOG("DEF_ASM...");
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







