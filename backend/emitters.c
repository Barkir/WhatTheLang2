#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "what_lang/tree.h"
#include "what_lang/emitters.h"

void PUSHIMM32(char ** buf,  FILE * file, field_t value)
{
    int val = (int) value;
    **buf = PUSHIMM32_BYTE;
    (*buf)++;
    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}


void PUSHREG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "push %s\n", Adr2Reg(reg, 0));
    **buf = PUSHREG_BYTE + reg;
    (*buf)++;
}

void PUSH_XTEND_REG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "push %s\n", Adr2Reg(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    (*buf)++;
    **buf = PUSHREG_BYTE + reg;
    (*buf)++;
}

void POPREG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "pop %s\n", Adr2Reg(reg, 0));
    (**buf) = POP_BYTE + reg;
    (*buf)++;
}

void POP_XTEND_REG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "pop %s\n", Adr2Reg(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    (*buf)++;
    **buf = POP_BYTE + reg;
    (*buf)++;
}

void MULREG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "mul %s\n", Adr2Reg(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}

void MUL_XTEND_REG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "mul %s\n", Adr2Reg(reg, 1));
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}


void DIVREG(char ** buf, int reg)
{
    fprintf(file, "div %s\n", Adr2Reg(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void DIV_XTEND_REG(char ** buf, FILE * file, int reg)
{
    fprintf(file, "div %s\n", Adr2Reg(reg, 1));
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void CMP_REG_REG(char ** buf, FILE * file, int reg1, int reg2, int mode)
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

    **buf = CMP_BYTE;
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm:
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
