#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>

#include "what_lang/nametable.h"

#include "what_lang/list.h"
#include "what_lang/htable.h"

#include "what_lang/tree.h"
#include "what_lang/parser.h"
#include "what_lang/backend.h"
#include "what_lang/emit_constants.h"
#include "what_lang/backend_utils.h"
#include "what_lang/emitters.h"
#include "what_lang/errors.h"
#include "what_lang/nasm2elf.h"

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

    fprintf(fp, "%s", NASM_TOP);

    for (int i = 0; names[i].name; i++)
    {
        for(int j = 0; func[j].name; j++)
        {
            if (!strcmp(names[i].name, func[j].name))
            {
                PARSER_LOG("FOUND FUNC %s", func[j].name);
                names[i].name = func[j].name;
                names[i].param = func[j].param;
                names[i].type = func[j].type;
                names[i].address = _find_func_start(names, names[i].name);
                names[i].address_end = _find_func_end(names, names[i].name);
                break;
            }
        }
    }

    char* buf = (char*) calloc(DEF_SIZE * 8, 1);
    if (!buf) return WHAT_MEMALLOC_ERROR;
    char* buf_ptr = buf;

    GenerateElfHeader(buf);
    buf += BUF_OFFSET;

    Htable * tab = NULL;
    HtableInit(&tab, HTABLE_BINS);

    _create_bin(&buf, &tab, names, tree->root, fp, 0, 0, 0, 0);

    EMIT_EXIT(&buf);

    PARSER_LOG("Bin created, in buf %10s", buf_ptr);
    if (mode == WHAT_DEBUG_MODE) HtableDump(tab);

    PARSER_LOG("Writing buf to file");
    fwrite(buf_ptr, DEF_SIZE * 8, sizeof(char), bin);

    fprintf(fp, "%s", NASM_BTM);

    _def_bin(&buf, &tab, names, tree->root, fp, 0, 0, 0, 0);


    free(names);
    fclose(fp);
    fclose(bin);

    return 0;

}

int _create_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    if (NodeType(root) == NUM)
    {
        PARSER_LOG("PUSHING IMM32");
        PUSHIMM32(buf, file, (int) NodeValue(root));
    }
    if (NodeType(root) == VAR)
    {
        int adr = GetVarAdr(root, names);
        if (adr >= 0)
        {
            PARSER_LOG("PUSHING REG...");
            PARSER_LOG("Variable Name = %s, adr = %d, reg = %d", NodeName(root), adr, Adr2EnumReg(adr));
            PUSHREG(buf, file, (uint8_t) Adr2EnumReg(adr));
        }
        else
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
    }

    if (NodeType(root) == OPER)
    {
        int local_if = 0;
        int local_while = 0;

        int nodeVal = (int) NodeValue(root);

        if      (isCmpOper(nodeVal))    BinCmpOper  (buf, tab, names, root, file, if_cond, while_cond, &if_count, &while_count);
        else if (isArithOper(nodeVal))  BinArithOper(buf, tab, names, root->left, file, if_cond, while_cond, &if_count, &while_count);
        else if (nodeVal == IF)         BinIf       (buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
        else if (nodeVal == WHILE)      BinWhile    (buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
    }
    if (NodeType(root) == FUNC)
    {
        switch((int) NodeValue(root))
        {
            case SIN:   _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sin\n");
                        break;

            case COS:   _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "cos\n");
                        break;

            case SQRT:  _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sqrt\n");
                        break;

            case PRINT: _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "pop rax\n");
                        fprintf(file, "call _IOLIB_OUTPUT\n");
                        break;

            case INPUT: fprintf(file, "call _IOLIB_INPUT\n");
                        fprintf(file, "push rax\n");
                        break;

            default: return -1;
        }
    }

    if (NodeType(root) == FUNC_NAME) fprintf(file, "call %s\n", NodeName(root));

    if ((int) NodeValue(root) == ';')  {_create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count); _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);}
    return 1;
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






