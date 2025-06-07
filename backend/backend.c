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
#include "what_lang/emitters.h"         // Emitters Functions

#include "what_lang/backend.h"          // Backend Header
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

    Name * names = CreateVarTable(tree->root);
    if (!names) return WHAT_VARTABLE_ERROR;

    Name * func  = CreateFuncTable(tree->root);
    if (!func) return WHAT_FUNCTABLE_ERROR;

    DefineFuncTable(&func, &names);

    char * buf = (char*) calloc(DEF_SIZE * 8, 1);
    if (!buf) return WHAT_MEMALLOC_ERROR;
    char * buf_ptr = buf;

    GenerateElfHeader(&buf);

    Htable * tab = NULL;
    HtableInit(&tab, HTABLE_BINS);

    fprintf(fp, "%s", NASM_TOP);
    _create_bin(&buf, &tab, names, tree->root, fp, 0, 0, 0, 0);
    fprintf(fp, "%s", NASM_BTM);

    EMIT_EXIT(&buf);

    PARSER_LOG("Bin created, in buf %10s", buf_ptr);

    if (mode == WHAT_DEBUG_MODE)
    {
        HtableDump(tab);
        TreeDump(tree, "dump");
    }


    _def_bin(&buf, &tab, names, tree->root, fp, 0, 0, 0, 0);

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

int _create_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    if (NodeType(root) == NUM)
    {
        PARSER_LOG("PUSHING IMM32");
        PUSHIMM32(buf, file, (int) NodeValue(root));
    }

    else if (NodeType(root) == VAR)
    {
        int adr = GetVarAdr(root, names);
        if (adr < 0)    // means it is a function
        {
            Name * func = GetFuncAdr(root, names);
            Node * dummy = root->left;

            if (func->param != 0)
            {
                for (int i = func->address; dummy; i++)
                {
                    PARSER_LOG("PUSHING IMM32");
                    PUSHIMM32   (buf, file, (int) NodeValue(dummy));
                    POPREG      (buf, file, (uint8_t) Adr2EnumReg(adr));

                    dummy = dummy->left;
                }
            }
            CALL_DIRECT(buf, file, NodeIP(root), NodeName(root));
        }
        else            // means it is a variable
        {
            PARSER_LOG("PUSHING REG...");
            PARSER_LOG("Variable Name = %s, adr = %d, reg = %d", NodeName(root), adr, Adr2EnumReg(adr));
            PUSHREG(buf, file, (uint8_t) Adr2EnumReg(adr));
        }
    }

    else if (NodeType(root) == OPER)
    {
        int local_if = 0;
        int local_while = 0;

        int nodeVal = (int) NodeValue(root);

        if      (isCmpOper(nodeVal))    BinCmpOper  (buf, tab, names, root, file, if_cond, while_cond, &if_count, &while_count);
        else if (isArithOper(nodeVal))  BinArithOper(buf, tab, names, root, file, if_cond, while_cond, &if_count, &while_count);
        else if (nodeVal == IF)         BinIf       (buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
        else if (nodeVal == WHILE)      BinWhile    (buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
    }

    else if (NodeType(root) == FUNC) BinFunc(buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
    else if (NodeType(root) == FUNC_NAME) fprintf(file, "call %s\n", NodeName(root));

    else if ((int) NodeValue(root) == ';')
    {
        _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
        _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);
    }

    return WHAT_SUCCESS;
}

int _def_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    PARSER_LOG("DEF_ASM...");
    if (NodeType(root) == OPER && (int) NodeValue(root) == DEF)
    {
        Node * param = root;
        int i = 0;

        fprintf(file, "%s:\n", NodeName(root->left));
        fprintf(file, "; pushing return address to stack\n");
        fprintf(file, "pop r14\n");
        fprintf(file, "mov [r13], r14\n");
        fprintf(file, "add r13, 8\n");

        _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

        fprintf(file, "; popping return address to stack\n");
        fprintf(file, "sub r13, 8\n");
        fprintf(file, "mov r14, [r13]\n");
        fprintf(file, "push r14\n");

        fprintf(file, "ret\n");
    }
    if (root->left)  _def_bin(buf, tab,names, root->left, file, if_cond, while_cond, if_count, while_count);
    if (root->right) _def_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);
    return 1;
}






