#ifndef EMITTERS_H
#define EMITTERS_H

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

void PUSHIMM32      (char ** buf,  field_t value);
void PUSHREG        (char ** buf, int reg);
void PUSH_XTEND_REG (char ** buf, int reg);
void POPREG         (char ** buf, int reg);
void POP_XTEND_REG  (char ** buf, int reg);

void MULREG         (char ** buf, int reg);

void MUL_XTEND_REG  (char ** buf, int reg);


void DIVREG         (char ** buf, int reg);

void DIV_XTEND_REG  (char ** buf, int reg);

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT    (char ** buf, int adr);

#endif
