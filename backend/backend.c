#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "what_lang/tree.h"
#include "what_lang/parser.h"
#include "what_lang/nametable.h"
#include "what_lang/backend.h"
#include "what_lang/errors.h"

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

int _create_bin(char ** buf, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    if (NodeType(root) == NUM)
    {
        PUSHIMM32(buf, NodeValue(root));
    }
    if (NodeType(root) == VAR)
    {
        int adr = GetVarAdr(root, names);
        if (adr >= 0)
        {
            PUSHREG(buf, Adr2EnumReg(adr));
        }
        else
        {
            Name * func = GetFuncAdr(root, names);
            Node * dummy = root->left;

            if (func->param != 0)
            {
                for (int i = func->address; dummy; i++)
                {
                    PUSHIMM32(buf, NodeValue(dummy));
                    POPREG(buf, Adr2EnumReg(adr));

                    dummy = dummy->left;
                }
            }
            CALL_DIRECT(buf, NodeIP(root));
        }
    }

    if (NodeType(root) == OPER)
    {
        int local_if = 0;
        int local_while = 0;

        switch ((int) NodeValue(root))
        {
            case '=':   _create_bin(buf, names, root->right, file, if_cond, while_cond, if_count, while_count);
                        POPREG(buf, Adr2EnumReg(GetVarAdr(root->left, names)));
                        break;

            case '+':   _create_bin(buf, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_bin(buf, names, root->right, file, if_cond, while_cond, if_count, while_count);


                        POP_XTEND_REG(buf, WHAT_REG_R14);
                        POP_XTEND_REG(buf, WHAT_REG_R15);

                        break;

            case '-':   _create_bin(buf, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_bin(buf, names, root->right, file, if_cond, while_cond, if_count, while_count);

                        POP_XTEND_REG(buf, WHAT_REG_R14);
                        POP_XTEND_REG(buf, WHAT_REG_R15);

                        break;

            case '*':   _create_bin(buf, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_bin(buf, names, root->right, file, if_cond, while_cond, if_count, while_count);

                        POP_XTEND_REG(buf, WHAT_REG_R14);
                        POPREG(buf, WHAT_REG_EAX);

                        MUL_XTEND_REG(buf, WHAT_REG_R14);
                        PUSHREG(buf, WHAT_REG_EAX);
                        break;

            case '/':   _create_bin(buf, names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_bin(buf, names, root->right, file, if_cond, while_cond, if_count, while_count);

                        POP_XTEND_REG(buf, WHAT_REG_R14);
                        POPREG(buf, WHAT_REG_EAX);


                        DIV_XTEND_REG(buf, WHAT_REG_R14);
                        PUSHREG(buf, WHAT_REG_EAX);
                        break;

            // TODO: функции под сравнения

            case MORE:      _create_bin(buf, names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_bin(buf, names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'>' comparsion\n\n");

                            POP_XTEND_REG(buf, WHAT_REG_R15);

                            POP_XTEND_REG(buf, WHAT_REG_R14);

                            PUSH_XTEND_REG(buf, WHAT_REG_R14);
                            PUSH_XTEND_REG(buf, WHAT_REG_R14);

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "ja SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "ja SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");

                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");
                            break;

            case LESS:      _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, "'<' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jb SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jb SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case MORE_E:    _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'>=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jae SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jae SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case LESS_E:    _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'<=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jbe SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jbe SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case EQUAL:     _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'==' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "je SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "je SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case N_EQUAL:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'!=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jne SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jne SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;


            case IF:
                        local_if = IF_COUNT;
                        if_count = IF_COUNT;
                        IF_COUNT++;

                        _create_asm(names, root->left, file, 1, 0, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";if condition\n\n");

                        fprintf(file, "IF%d:\n", if_count);
                        fprintf(file, "push 0\n");


                        fprintf(file, "pop r15\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, "push r15\n");

                        fprintf(file, "cmp r14, r15\n");

                        fprintf(file, "jne COND%d\n", if_count);

                        fprintf(file, "jmp IF_END%d\n", if_count);

                        fprintf(file, "COND%d:\n", if_count);
                        if_count++;
                        _create_asm(names, root->right, file, 1, 0, if_count, while_count);

                        fprintf(file, "IF_END%d:\n", local_if);

                        fprintf(file, ";---------------------------\n\n");
                        break;


            case WHILE:
                        local_while = WHILE_COUNT;
                        while_count = WHILE_COUNT;
                        WHILE_COUNT++;

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";while condition\n\n");
                        fprintf(file, "WHILE%d:\n", while_count);
                        _create_asm(names, root->left, file, 0, 1, if_count, while_count);

                        fprintf(file, "WHILE_FALSE%d:\n", while_count);
                        fprintf(file, "push 0\n");

                        fprintf(file, "pop r15\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, "push r15\n");

                        fprintf(file, "cmp r14, r15\n");

                        fprintf(file, "je WHILE_END%d\n", while_count);

                        fprintf(file, "WHILE_TRUE%d:\n", while_count);
                        _create_asm(names, root->right, file, 0, 1, if_count, while_count);

                        fprintf(file, "jmp WHILE%d\n", local_while);
                        fprintf(file, "WHILE_END%d:\n", local_while);

                        fprintf(file, ";---------------------------\n\n");
                        break;

            case DEF:   break;


            default: return -1;


        }
    }
    if (NodeType(root) == FUNC)
    {
        switch((int) NodeValue(root))
        {
            case SIN:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sin\n");
                        break;

            case COS:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "cos\n");
                        break;

            case SQRT:  _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sqrt\n");
                        break;

            case PRINT: _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "out\n");
                        break;

            case INPUT: fprintf(file, "in\n");
                        break;

            default: return -1;
        }
    }

    if (NodeType(root) == FUNC_NAME) fprintf(file, "call %s\n", NodeName(root));

    if ((int) NodeValue(root) == ';')  {_create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count); _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);}
    return 1;
}

int _def_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
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

        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

        fprintf(file, "; popping return address to stack\n");
        fprintf(file, "sub r13, 8\n");
        fprintf(file, "mov r14, [r13]\n");
        fprintf(file, "push r14\n");

        fprintf(file, "ret\n");
    }
    if (root->left)  _def_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
    if (root->right) _def_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);
    return 1;
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

int CreateAsm(Tree * tree, const char * filename)
{
    PARSER_LOG("Creating ASM....");
    FILE * fp = fopen(filename, "wb");
    if (ferror(fp)) return -1;

    Name * names = CreateVarTable(tree->root);
    ADR_COUNT = 0;

    Name * func  = CreateFuncTable(tree->root);

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
    if (!names) return -1;

    for (int i = 0; names[i].name; i++)
    {
        // printf("%s(param: %d, type: %d, func_name = %s): starts at %d, ends at %d\n", names[i].name, names[i].param, names[i].type, names[i].func_name, names[i].address, names[i].address_end);
    }

    fprintf(fp, "section .text\n");
    fprintf(fp, "global _start\n");
    fprintf(fp, "_start:\n");
    fprintf(fp, "mov r13, stack4calls\n");
    _create_asm(names, tree->root, fp, 0, 0, 0, 0);

    fprintf(fp, "mov rax, 60\n");
    fprintf(fp, "syscall\n");

    _def_asm(names, tree->root, fp, 0, 0, 0, 0);

    fprintf(fp, "section .data\n");
    fprintf(fp, "stack4calls times 128 * 8 db 0  ; stack for calls");

    free(names);
    fclose(fp);
    return 0;

}

int CreateBin(Tree * tree, const char * filename)
{
    PARSER_LOG("Creating BIN....");
    FILE * fp = fopen(filename, "wb");
    if (ferror(fp)) return -1;

    Name * names = CreateVarTable(tree->root);
    ADR_COUNT = 0;

    Name * func  = CreateFuncTable(tree->root);

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
    if (!names) return -1;

    for (int i = 0; names[i].name; i++)
    {
        // printf("%s(param: %d, type: %d, func_name = %s): starts at %d, ends at %d\n", names[i].name, names[i].param, names[i].type, names[i].func_name, names[i].address, names[i].address_end);
    }

    char* buf = (char*) calloc(DEF_SIZE * 4, 1);
    _create_bin(&buf, names, tree->root, fp, 0, 0, 0, 0);

    fprintf(fp, "mov rax, 60\n");
    fprintf(fp, "syscall\n");

    _def_asm(names, tree->root, fp, 0, 0, 0, 0);

    fprintf(fp, "section .data\n");
    fprintf(fp, "stack4calls times 128 * 8 db 0  ; stack for calls");

    free(names);
    fclose(fp);
    return 0;

}

int _create_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    if (NodeType(root) == NUM)
    {
        fprintf(file, "push %lg\n", NodeValue(root));
    }
    if (NodeType(root) == VAR)
    {
        int adr = GetVarAdr(root, names);
        if (adr >= 0) fprintf(file, "push %s\n", Adr2Reg(adr));
        else
        {
            Name * func = GetFuncAdr(root, names);
            Node * dummy = root->left;

            if (func->param != 0)
            {
                for (int i = func->address; dummy; i++)
                {
                    fprintf(file, "push %lg\n", NodeValue(dummy));
                    fprintf(file, "pop %s\n", Adr2Reg(names[i].address));
                    dummy = dummy->left;
                }
            }
            fprintf(file, "call %s\n", NodeName(root));
        }
    }

    if (NodeType(root) == OPER)
    {
        int local_if = 0;
        int local_while = 0;

        switch ((int) NodeValue(root))
        {
            case '=':   _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "pop %s             ; '=' operation\n", Adr2Reg(GetVarAdr(root->left, names)));
                        break;

            case '+':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'+' operation\n\n");

                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop r15\n");

                        fprintf(file, "add r14, r15\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case '-':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'-' operation\n\n");

                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop r15\n");

                        fprintf(file, "sub r15, r14\n");
                        fprintf(file, "push r15\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case '*':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'*' operation\n\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop rax\n");

                        fprintf(file, "mul r14\n");
                        fprintf(file, "push rax\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case '/':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'/' operation\n\n");

                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop rax\n");


                        fprintf(file, "div r14\n");
                        fprintf(file, "push rax\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case MORE:      _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'>' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "ja SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "ja SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");

                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");
                            break;

            case LESS:      _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, "'<' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jb SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jb SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case MORE_E:    _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'>=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jae SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jae SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case LESS_E:    _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'<=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jbe SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jbe SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case EQUAL:     _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'==' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "je SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "je SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case N_EQUAL:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'!=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jne SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jne SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;


            case IF:
                        local_if = IF_COUNT;
                        if_count = IF_COUNT;
                        IF_COUNT++;

                        _create_asm(names, root->left, file, 1, 0, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";if condition\n\n");

                        fprintf(file, "IF%d:\n", if_count);
                        fprintf(file, "push 0\n");


                        fprintf(file, "pop r15\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, "push r15\n");

                        fprintf(file, "cmp r14, r15\n");

                        fprintf(file, "jne COND%d\n", if_count);

                        fprintf(file, "jmp IF_END%d\n", if_count);

                        fprintf(file, "COND%d:\n", if_count);
                        if_count++;
                        _create_asm(names, root->right, file, 1, 0, if_count, while_count);

                        fprintf(file, "IF_END%d:\n", local_if);

                        fprintf(file, ";---------------------------\n\n");
                        break;


            case WHILE:
                        local_while = WHILE_COUNT;
                        while_count = WHILE_COUNT;
                        WHILE_COUNT++;

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";while condition\n\n");
                        fprintf(file, "WHILE%d:\n", while_count);
                        _create_asm(names, root->left, file, 0, 1, if_count, while_count);

                        fprintf(file, "WHILE_FALSE%d:\n", while_count);
                        fprintf(file, "push 0\n");

                        fprintf(file, "pop r15\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, "push r15\n");

                        fprintf(file, "cmp r14, r15\n");

                        fprintf(file, "je WHILE_END%d\n", while_count);

                        fprintf(file, "WHILE_TRUE%d:\n", while_count);
                        _create_asm(names, root->right, file, 0, 1, if_count, while_count);

                        fprintf(file, "jmp WHILE%d\n", local_while);
                        fprintf(file, "WHILE_END%d:\n", local_while);

                        fprintf(file, ";---------------------------\n\n");
                        break;

            case DEF:   break;


            default: return -1;


        }
    }
    if (NodeType(root) == FUNC)
    {
        switch((int) NodeValue(root))
        {
            case SIN:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sin\n");
                        break;

            case COS:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "cos\n");
                        break;

            case SQRT:  _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sqrt\n");
                        break;

            case PRINT: _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "out\n");
                        break;

            case INPUT: fprintf(file, "in\n");
                        break;

            default: return -1;
        }
    }

    if (NodeType(root) == FUNC_NAME) fprintf(file, "call %s\n", NodeName(root));

    if ((int) NodeValue(root) == ';')  {_create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count); _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);}
    return 1;
}

const char * Adr2Reg(int adr)
{
    switch(adr)
    {
        case 0:     return "rbx";
        case 1:     return "rcx";
        case 2:     return "rdx";
        case 3:     return "rsi";
        case 4:     return "rdi";
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
        default:    return WHAT_REG_EBX;
    }

}

void PUSHIMM32(char ** buf,  field_t value)
{
    int val = (int) value;
    **buf = PUSHIMM32_BYTE;
    (*buf)++;
    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}


void PUSHREG(char ** buf, int reg)
{
    **buf = PUSHREG_BYTE + reg;
    (*buf)++;
}

void PUSH_XTEND_REG(char ** buf, int reg)
{
    **buf = ADDITIONAL_REG_BYTE;
    (*buf)++;
    **buf = PUSHREG_BYTE + reg;
    (*buf)++;
}

void POPREG(char ** buf, int reg)
{
    (**buf) = POP_BYTE + reg;
    (*buf)++;
}

void POP_XTEND_REG(char ** buf, int reg)
{
    **buf = ADDITIONAL_REG_BYTE;
    (*buf)++;
    **buf = POP_BYTE + reg;
    (*buf)++;
}

void MULREG(char ** buf, int reg)
{
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}

void MUL_XTEND_REG(char ** buf, int reg)
{
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}


void DIVREG(char ** buf, int reg)
{
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void DIV_XTEND_REG(char ** buf, int reg)
{
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT(char ** buf, int adr)
{
    **buf = CALL_DIRECT_BYTE;
    (*buf)++;
    memcpy(*buf, ((*buf)-adr), sizeof(int));
    (*buf) += sizeof(int);
}
