#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "what_lang/nametable.h"

#include "what_lang/list.h"
#include "what_lang/htable.h"

#include "what_lang/tree.h"
#include "what_lang/backend.h"
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
    PARSER_LOG("PUSHING 2 REG %d", reg);
    fprintf(file, "push %s\n", Adr2Reg(reg, 0));
    **buf = PUSHREG_BYTE + reg;
    PARSER_LOG("VALUE IN BUF %d", **buf);
    (*buf)++;
}

void PUSH_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "push %s\n", Adr2Reg(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    (*buf)++;
    **buf = PUSHREG_BYTE + reg;
    (*buf)++;
}

void POPREG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "pop %s\n", Adr2Reg(reg, 0));
    (**buf) = POP_BYTE + reg;
    (*buf)++;
}

void POP_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "pop %s\n", Adr2Reg(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    (*buf)++;
    **buf = POP_BYTE + reg;
    (*buf)++;
}

void MULREG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "mul %s\n", Adr2Reg(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}

void MUL_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "mul %s\n", Adr2Reg(reg, 1));
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}


void DIVREG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "div %s\n", Adr2Reg(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void DIV_XTEND_REG(char ** buf, FILE * file, uint8_t reg)
{
    fprintf(file, "div %s\n", Adr2Reg(reg, 1));
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
        case WHAT_REG_REG:      fprintf(file, "cmp %s, %s\n", Adr2Reg(reg1, 0), Adr2Reg(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(file, "cmp %s, %s\n", Adr2Reg(reg1, 0), Adr2Reg(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(file, "cmp %s, %s\n", Adr2Reg(reg1, 1), Adr2Reg(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(file, "cmp %s, %s\n", Adr2Reg(reg1, 1), Adr2Reg(reg2, 1));
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

    uint8_t fbyte = (mode == WHAT_REG_VAL) ? OPER_BYTE : XTEND_OPER_BYTE;

    if (fbyte == OPER_BYTE) fprintf(file, "add %s, %d\n", Adr2Reg(reg, 0), add_value);
    else                    fprintf(file, "add %s, %d\n", Adr2Reg(reg, 1), add_value);

    **buf = fbyte;
    (*buf)++;


    if (reg == WHAT_REG_EAX)
    {
        **buf = 0x5;
        (*buf)++;
    }

    else
    {
        uint8_t modrm = (0xc0) | (0 << 3) | (reg & 7);
        **buf = ADD_REG_VAL_BYTE;
        (*buf)++;
        **buf = modrm;
        (*buf)++;
    }

    memcpy(*buf, &add_value, sizeof(int));
    (*buf) += sizeof(int);
}

void ADD_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{

    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(file, "add %s, %s\n", Adr2Reg(reg1, 0), Adr2Reg(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(file, "add %s, %s\n", Adr2Reg(reg1, 0), Adr2Reg(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(file, "add %s, %s\n", Adr2Reg(reg1, 1), Adr2Reg(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(file, "add %s, %s\n", Adr2Reg(reg1, 1), Adr2Reg(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = ADD_REG_REG_BYTE;
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
        case WHAT_REG_REG:      fprintf(file, "sub %s, %s\n", Adr2Reg(reg1, 0), Adr2Reg(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(file, "sub %s, %s\n", Adr2Reg(reg1, 0), Adr2Reg(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(file, "sub %s, %s\n", Adr2Reg(reg1, 1), Adr2Reg(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(file, "sub %s, %s\n", Adr2Reg(reg1, 1), Adr2Reg(reg2, 1));
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
    (*buf)++;
    **buf = offset;
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

