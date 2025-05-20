#ifndef EMITTERS_H
#define EMITTERS_H

enum RegModes
{
    WHAT_REG_REG,
    WHAT_XTEND_REG,
    WHAT_REG_XTEND,
    WHAT_XTEND_XTEND
};



const static char VAL2REG_START = 0xb8;
const static char PUSHREG_BYTE = 0x50;
const static char PUSHIMM32_BYTE = 0x68;
const static char POP_BYTE  = 0x59;
const static char CALL_DIRECT_BYTE = 0xe8;
const static char ADDITIONAL_REG_BYTE = 0x41;
const static char MULREG_BYTE = 0xe0;
const static char DIVREG_BYTE = 0xf0;
const static char OPER_BYTE = 0x48;
const static char XTEND_OPER_BYTE = 0x49;

const static REG_REG_BYTE       = 0x48;
const static XTEND_REG_BYTE     = 0x49;
const static REG_XTEND_BYTE     = 0x4c;
const static XTEND_XTEND_BYTE   = 0x4d;

const static CMP_BYTE           = 0x39;

void PUSHIMM32      (char ** buf, FILE * file, field_t value);
void PUSHREG        (char ** buf, FILE * file, int reg);
void PUSH_XTEND_REG (char ** buf, FILE * file, int reg);
void POPREG         (char ** buf, FILE * file, int reg);
void POP_XTEND_REG  (char ** buf, FILE * file, int reg);
void MULREG         (char ** buf, FILE * file, int reg);
void MUL_XTEND_REG  (char ** buf, FILE * file, int reg);
void DIVREG         (char ** buf, FILE * file, int reg);
void DIV_XTEND_REG  (char ** buf, FILE * file, int reg);

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT    (char ** buf, FILE * file, int adr);

#endif
