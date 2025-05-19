#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "what_lang/tree.h"
#include "what_lang/emitters.h"

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
