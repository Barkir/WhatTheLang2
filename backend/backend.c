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
#include "what_lang/backend_utils.h"
#include "what_lang/emitters.h"
#include "what_lang/emit_constants.h"
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

        switch ((int) NodeValue(root))
        {
            case '=':       _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);    PARSER_LOG("OPER '='");
                            POPREG(buf, file, Adr2EnumReg(GetVarAdr(root->left, names)));
                            break;

            case '+':       _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);     PARSER_LOG("OPER '+'");
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);


                            POP_XTEND_REG(buf, file, WHAT_REG_R14);
                            POP_XTEND_REG(buf, file, WHAT_REG_R15);

                            ADD_REG_REG(buf, file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);
                            PUSH_XTEND_REG(buf, file, WHAT_REG_R14);
                            break;

            case '-':       _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);     PARSER_LOG("OPER '-'");
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            POP_XTEND_REG(buf, file, WHAT_REG_R15);
                            POP_XTEND_REG(buf, file, WHAT_REG_R14);

                            SUB_REG_REG(buf, file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);
                            PUSH_XTEND_REG(buf, file, WHAT_REG_R14);

                            break;

            case '*':       _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);     PARSER_LOG("OPER '*'");
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            POP_XTEND_REG(buf, file, WHAT_REG_R14);
                            POPREG(buf, file, WHAT_REG_EAX);

                            MUL_XTEND_REG(buf, file, WHAT_REG_R14);
                            PUSHREG(buf, file, WHAT_REG_EAX);
                            break;

            case '/':       _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);     PARSER_LOG("OPER '/'");
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            POP_XTEND_REG(buf, file, WHAT_REG_R14);
                            POPREG(buf, file, WHAT_REG_EAX);


                            DIV_XTEND_REG(buf, file, WHAT_REG_R14);
                            PUSHREG(buf, file, WHAT_REG_EAX);
                            break;

            case MORE:      _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            EMIT_COMPARSION(buf, file, tab, JA_BYTE, "ja", &if_count, &while_count, if_cond, while_cond);

                            break;

            case LESS:      _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            EMIT_COMPARSION(buf, file, tab, JB_BYTE, "jb", &if_count, &while_count, if_cond, while_cond);

                            break;

            case MORE_E:    _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            EMIT_COMPARSION(buf, file, tab, JAE_BYTE, "jae", &if_count, &while_count, if_cond, while_cond);

                            break;

            case LESS_E:    _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            EMIT_COMPARSION(buf, file, tab, JBE_BYTE, "jbe", &if_count, &while_count, if_cond, while_cond);

                            break;

            case EQUAL:     _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            EMIT_COMPARSION(buf, file, tab, JE_BYTE, "je", &if_count, &while_count, if_cond, while_cond);

                            break;

            case N_EQUAL:   _create_bin(buf, tab, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, tab, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            EMIT_COMPARSION(buf, file, tab, JNE_BYTE, "jne", &if_count, &while_count, if_cond, while_cond);

                            break;

            case IF:        _if_bin(buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
                            break;


            case WHILE:     _while_bin(buf, tab, names, root, file, if_cond, while_cond, if_count, while_count);
                            break;

            case DEF:       break;


            default: return -1;


        }
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

int _if_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
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
    Name * label = HtableNameFind(*tab, &locals_if);
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
    label = HtableNameFind(*tab, &locals_if);
    if (label) *label->offset = (int8_t)(*buf - (label->offset + 1));
    else return WHAT_NOLABEL_ERROR;

    return WHAT_SUCCESS;

}

int _while_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
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
    Name * label_while = HtableNameFind(*tab, &locals_while);
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
    label_while = HtableNameFind(*tab, &locals_while);
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
}






