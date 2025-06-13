#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <assert.h>

#include "what_lang/nametable.h"

#include "what_lang/list.h"             // List Structure
#include "what_lang/htable.h"           // Hash Table (includes list)

#include "what_lang/tree.h"             // Binary Tree Structure
#include "what_lang/parser.h"           // Included for operations enum (bad)

#include "what_lang/errors.h"           // Errors and loggers

#include "what_lang/emit_constants.h"   // Emitters Constants
#include "what_lang/backend.h"          // Backend Header
#include "what_lang/emitters.h"         // Emitters Functions

#include "what_lang/backend_utils.h"    // Backend Utils Header

#include "what_lang/nasm2elf.h"         // what the hell is that?

int CreateBin(Tree * tree, const char * filename_asm, const char * filename_bin, enum RunModes mode)
{
    PARSER_LOG("Creating BIN....");
    FILE * fp = fopen(filename_asm, "wb");
    if (ferror(fp))
    {
        perror("Error opening file fp");
        return WHAT_FILEOPEN_ERROR;
    }

    FILE * bin = fopen(filename_bin, "wb");
    if (ferror(bin))
    {
        perror("Error opening file what.out");
        return WHAT_FILEOPEN_ERROR;
    }

    Htable * names = CreateNameTable(tree->root);
    HtableDump(names);
    PARSER_LOG("Created NameTable");
    TreeDump(tree, "dump");

    char * buf = (char*) calloc(DEF_SIZE * 8, 1);
    if (!buf) return WHAT_MEMALLOC_ERROR;
    char * buf_ptr = buf;

    GenerateElfHeader(&buf);

    Htable * tab = NULL;
    HtableInit(&tab, HTABLE_BINS);

    fprintf(fp, "%s", NASM_TOP);

    BinCtx ctx = {.file = fp, .names = names, .func_name = GLOBAL_FUNC_NAME};
    _create_bin(&buf, &tab, tree->root, &ctx);
    fprintf(fp, "%s", NASM_BTM);
    EMIT_EXIT(&buf);



    PARSER_LOG("Bin created, in buf %10s", buf_ptr);

    if (mode == WHAT_DEBUG_MODE)
    {
        HtableDump(tab);
    }


    _def_bin(&buf, &tab, tree->root, &ctx);
    HtableDump(names);

    PARSER_LOG("Writing buf to file");
    size_t err_chk = 0;
    assert((err_chk = fwrite(buf_ptr, sizeof(char), DEF_SIZE * 8, bin)) == DEF_SIZE * 8);

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

int _create_bin(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    if (NodeType(root) == NUM)
    {
        PARSER_LOG("PUSHING IMM32");
        PUSHIMM32(buf, (int) NodeValue(root), ctx);
    }

    else if (NodeType(root) == FUNC_INTER_CALL)
    {
        PARSER_LOG("FUNC INTER_CALL");

        Name func = {.name = NodeName(root), .type=FUNC_INTER_DEF};
        Name ** param_array = HtableNameFind(ctx->names, &func)->name_array;

        int param = 0;
        for (Node * left = root->left; left; left = left->left, param++)
        {
            if (NodeType(left) == NUM)
            {
                fprintf(ctx->file, "push r12            \n");
                fprintf(ctx->file, "push rax            \n");
                fprintf(ctx->file, "add r12, %d * 8     \n", param_array[param]->stack_offset);
                fprintf(ctx->file, "mov rax, %d         \n", (int) NodeValue(left));
                fprintf(ctx->file, "mov [r12], rax      \n");
                fprintf(ctx->file, "pop rax             \n");
                fprintf(ctx->file, "pop r12             \n");
                PUSHIMM32(buf, (int) NodeValue(left), ctx);
            }
            else if (NodeType(left) == VAR)
            {
                if (!strcmp(NodeName(root), ctx->func_name))
                {
                    fprintf(ctx->file, "push %s \n", Offset2StrReg(param, 0));
                }
                else
                {
                    fprintf(ctx->file, "push r12            \n");
                    fprintf(ctx->file, "add r12, %d * 8     \n", GetVarOffset(left, ctx));
                    fprintf(ctx->file, "mov %s, [r12]       \n", Offset2StrReg(param, 0));
                    fprintf(ctx->file, "pop r12             \n");
                    fprintf(ctx->file, "push %s             \n", Offset2StrReg(param, 0));
                }
            }
            PARSER_LOG("param = %d %s", param, param_array[param]->name);
            fprintf(ctx->file, "pop %s\n", Offset2StrReg(param_array[param]->param, 0));
        }

        fprintf(ctx->file, "call %s\n", NodeName(root));
        ctx->func_name = NodeName(root);
    }
    else if (NodeType(root) == VAR)
    {
        if (!strcmp(GetVarFuncName(root, ctx), GLOBAL_FUNC_NAME))
        {
            fprintf(ctx->file, "push r12            \n");
            fprintf(ctx->file, "add r12, %d * 8     \n", GetVarOffset(root, ctx));
            fprintf(ctx->file, "mov %s, [r12]       \n", Offset2StrReg(GetVarOffset(root, ctx), 0));
            fprintf(ctx->file, "pop r12             \n");
            fprintf(ctx->file, "push %s             \n", Offset2StrReg(GetVarOffset(root, ctx), 0));
        }
        else
        {
            fprintf(ctx->file, "push %s             \n", Offset2StrReg(GetVarParam(root, ctx), 0));
        }
    }
    else if (NodeType(root) == OPER)
    {
        PARSER_LOG("PROCESSING OPER");
        int local_if = 0;
        int local_while = 0;

        int nodeVal = (int) NodeValue(root);

        if      (isCmpOper(nodeVal))    BinCmpOper  (buf, tab, root, ctx);
        else if (isArithOper(nodeVal))  BinArithOper(buf, tab, root, ctx);
        else if (nodeVal == IF)         BinIf       (buf, tab, root, ctx);
        else if (nodeVal == WHILE)      BinWhile    (buf, tab, root, ctx);
    }
    else if (NodeType(root) == FUNC_EXT) BinFuncExt(buf, tab, root, ctx);
    else if ((int) NodeValue(root) == ';')
    {
        _create_bin(buf, tab, root->left,  ctx);
        _create_bin(buf, tab, root->right, ctx);
    }

    return WHAT_SUCCESS;
}

int _def_bin(char ** buf, Htable ** tab, Node * root, BinCtx * ctx)
{
    PARSER_LOG("DEF_ASM...");
    if (NodeType(root) == OPER && (int) NodeValue(root) == DEF)
    {

        fprintf(ctx->file, "%s:\n", NodeName(root->left));
        fprintf(ctx->file, "%s", RET_PUSH_STR);

        _create_bin(buf, tab, root->right, ctx);

        fprintf(ctx->file, "%s", RET_POP_STR);
    }
    if (root->left)  _def_bin(buf, tab, root->left,  ctx);
    if (root->right) _def_bin(buf, tab, root->right, ctx);
    return 1;
}






