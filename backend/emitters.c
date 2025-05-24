#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "what_lang/nametable.h"

#include "what_lang/list.h"
#include "what_lang/htable.h"

#include "what_lang/tree.h"
#include "what_lang/backend.h"
#include "what_lang/backend_utils.h"

#include "what_lang/emitters.h"
#include "what_lang/emit_constants.h"
#include "what_lang/errors.h"

void PUSHIMM32(char ** buf,  FILE * file, field_t value)
{
    int val = (int) value;
    fprintf(file, "push %d\n", val);
    PARSER_LOG("PUSHING VALUE %d", val);
    **buf = PUSHIMM32_BYTE;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;
    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}

void PUSHREG(char ** buf, FILE * file, uint8_t reg)
{
    PARSER_LOG("PUSHING REG %d %s", reg, Reg2Str(reg, 0));
    fprintf(file, "push %s\n", Reg2Str(reg, 0));
    **buf = PUSHREG_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", PUSHREG_BYTE, reg, **buf);
    (*buf)++;
}

void PUSH_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    PARSER_LOG("PUSHING REG %d %s", reg, Reg2Str(reg, 1));
    fprintf(file, "push %s\n", Reg2Str(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;
    **buf = PUSHREG_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", PUSHREG_BYTE, reg, **buf);
    (*buf)++;
}

void POPREG(char ** buf, FILE * file, uint8_t reg)
{
    PARSER_LOG("POPPING REG %d %s", reg, Reg2Str(reg, 0));
    fprintf(file, "pop %s\n", Reg2Str(reg, 0));
    (**buf) = POP_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", POP_BYTE, reg, **buf);
    (*buf)++;
}

void POP_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    PARSER_LOG("POPPING REG %d %s", reg, Reg2Str(reg, 1));
    fprintf(file, "pop %s\n", Reg2Str(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;
    **buf = POP_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", POP_BYTE, reg, **buf);
    (*buf)++;
}

void MULREG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "mul %s\n", Reg2Str(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}

void MUL_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "mul %s\n", Reg2Str(reg, 1));
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}


void DIVREG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "div %s\n", Reg2Str(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void DIV_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "div %s\n", Reg2Str(reg, 1));
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void CMP_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(file, "cmp %s, %s\n", Reg2Str(reg1, 0), Reg2Str(reg2, 0));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 0), reg2, Reg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(file, "cmp %s, %s\n", Reg2Str(reg1, 0), Reg2Str(reg2, 1));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 0), reg2, Reg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(file, "cmp %s, %s\n", Reg2Str(reg1, 1), Reg2Str(reg2, 0));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 1), reg2, Reg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(file, "cmp %s, %s\n", Reg2Str(reg1, 1), Reg2Str(reg2, 1));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 1), reg2, Reg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = CMP_REG_BYTE;
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm;
    (*buf)++;

}

void ADD_REG_VAL(char ** buf, FILE * file, uint8_t reg, field_t val, enum RegModes mode)
{
    int add_value = (int) val;

    if (mode == WHAT_REG_VAL) {PARSER_LOG("ADDING VALUE %d to reg %d %s", val, reg, Reg2Str(reg, 0));}
    else                      {PARSER_LOG("ADDING VALUE %d to reg %d %s", val, reg, Reg2Str(reg, 1));}

    uint8_t fbyte = (mode == WHAT_REG_VAL) ? OPER_BYTE : XTEND_OPER_BYTE;

    if (fbyte == OPER_BYTE) fprintf(file, "add %s, %d\n", Reg2Str(reg, 0), add_value);
    else                    fprintf(file, "add %s, %d\n", Reg2Str(reg, 1), add_value);

    **buf = fbyte;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;


    if (reg == WHAT_REG_EAX)
    {
        **buf = 0x5;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
    }

    else
    {
        uint8_t modrm = (0xc0) | (0 << 3) | (reg & 7);
        **buf = ADD_REG_VAL_BYTE;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
        **buf = modrm;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
    }

    memcpy(*buf, &add_value, sizeof(int));
    (*buf) += sizeof(int);
}

void ADD_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(file, "add %s, %s\n", Reg2Str(reg1, 0), Reg2Str(reg2, 0));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 0), reg2, Reg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(file, "add %s, %s\n", Reg2Str(reg1, 0), Reg2Str(reg2, 1));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 0), reg2, Reg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(file, "add %s, %s\n", Reg2Str(reg1, 1), Reg2Str(reg2, 0));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 1), reg2, Reg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(file, "add %s, %s\n", Reg2Str(reg1, 1), Reg2Str(reg2, 1));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, Reg2Str(reg1, 1), reg2, Reg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = ADD_REG_REG_BYTE;
    PARSER_LOG("OPCODE %x", ADD_REG_REG_BYTE);
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm;
    (*buf)++;
}

void SUB_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{

    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(file, "sub %s, %s\n", Reg2Str(reg1, 0), Reg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(file, "sub %s, %s\n", Reg2Str(reg1, 0), Reg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(file, "sub %s, %s\n", Reg2Str(reg1, 1), Reg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(file, "sub %s, %s\n", Reg2Str(reg1, 1), Reg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = SUB_REG_REG_BYTE;
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm;
    (*buf)++;
}


void EMIT_JMP(char ** buf, char jmp, char offset)
{
    **buf = jmp;
    PARSER_LOG("EMITTING JMP %x", **buf);
    (*buf)++;
    **buf = offset;
    PARSER_LOG("OFFSET %x", offset);
    (*buf)++;
}

void EMIT_EXIT(char ** buf)
{
    **buf = 0xb8;
    (*buf)++;
    **buf = 0x3c;
    (*buf) += sizeof(int);
    **buf = 0x0f;
    (*buf)++;
    **buf = 0x05;
    (*buf)++;
}

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT(char ** buf, FILE * file, int adr, const char * name)
{

    fprintf(file, "call %s\n", name);
    **buf = CALL_DIRECT_BYTE;
    (*buf)++;
    memcpy(*buf, ((*buf)-adr), sizeof(int));
    (*buf) += sizeof(int);
}


void EMIT_COMPARSION(char ** buf, FILE * file, Htable ** tab, char oper, const char * cond_jmp, int * if_count, int * while_count, int if_cond, int while_cond)
{
    char * offset = 0;

    Name locals = {};
    locals.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));

    POP_XTEND_REG   (buf,   file, WHAT_REG_R15);
    POP_XTEND_REG   (buf,   file, WHAT_REG_R14);
    PUSH_XTEND_REG  (buf,   file, WHAT_REG_R14);
    PUSH_XTEND_REG  (buf,   file, WHAT_REG_R14);
    CMP_REG_REG     (buf,   file, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);

    if (if_cond)
    {
        fprintf(file, "%s SUB_COND%d\n", cond_jmp, *if_count);
        EMIT_JMP(buf, oper, 0);
    }

    else if (while_cond)
    {
        fprintf(file, "%s SUB_COND%d\n", cond_jmp, *while_count);
        EMIT_JMP(buf, oper, 0);
    }

    offset = *buf - 1;

    PUSHIMM32(buf, file, 0);

    if (if_cond)
    {
        fprintf(file, "jmp IF_END%d\n", *if_count);
        EMIT_JMP(buf, JMP_BYTE, 0);

        sprintf(locals.local_func_name, "IF_END%d", *if_count);
        locals.offset = *buf - 1;

        PARSER_LOG("Inserting local label %s with offset %p", locals.local_func_name, locals.offset);
        HtableNameInsert(tab, &locals);

    }

    else if (while_cond)
    {
        fprintf(file, "jmp WHILE_FALSE%d\n", *while_count);
        EMIT_JMP(buf, JMP_BYTE, 0);

        sprintf(locals.local_func_name, "WHILE_FALSE%d", *while_count);
        locals.offset = *buf - 1;
        HtableNameInsert(tab, &locals);
    }

    if          (if_cond)    fprintf(file, "SUB_COND%d:\n",    *if_count);
    else if     (while_cond) fprintf(file, "SUB_COND%d:\n", *while_count);
    *offset = (int8_t)(*buf - (offset + 1));

    PUSHIMM32(buf, file, 1);

    if (if_cond)
    {
        PARSER_LOG("Inserting IF%d", *if_count);
        fprintf(file, "jmp IF%d\n", (*if_count));
        EMIT_JMP(buf, JMP_BYTE, 0);

        sprintf(locals.local_func_name, "IF%d", (*if_count)++);
        locals.offset = *buf - 1;
        HtableNameInsert(tab, &locals);
    }

    else if (while_cond)
    {
        fprintf(file, "jmp WHILE_TRUE%d\n", (*while_count));
        EMIT_JMP(buf, JMP_BYTE, 0);

        sprintf(locals.local_func_name, "WHILE_TRUE%d", (*while_count)++);
        locals.offset = *buf - 1;
        HtableNameInsert(tab, &locals);
    }
}

