#ifndef EMITTERS_H
#define EMITTERS_H

enum RegModes
{
    WHAT_REG_REG,
    WHAT_XTEND_REG,
    WHAT_REG_XTEND,
    WHAT_XTEND_XTEND,
    WHAT_REG_VAL,
    WHAT_XTEND_VAL

};

void PUSHIMM32      (char ** buf, FILE * file, field_t value);
void PUSHREG        (char ** buf, FILE * file, uint8_t reg);
void PUSH_XTEND_REG (char ** buf, FILE * file, uint8_t reg);
void POPREG         (char ** buf, FILE * file, uint8_t reg);
void POP_XTEND_REG  (char ** buf, FILE * file, uint8_t reg);
void MULREG         (char ** buf, FILE * file, uint8_t reg);
void MUL_XTEND_REG  (char ** buf, FILE * file, uint8_t reg);
void DIVREG         (char ** buf, FILE * file, uint8_t reg);
void DIV_XTEND_REG  (char ** buf, FILE * file, uint8_t reg);

void CMP_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode);
void ADD_REG_VAL(char ** buf, FILE * file, uint8_t reg, field_t val, enum RegModes mode);
void ADD_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode);
void SUB_REG_REG(char ** buf, FILE * file, uint8_t reg1, uint8_t reg2, enum RegModes mode);
void EMIT_JMP(char ** buf, char jmp, char offset);
void EMIT_COMPARSION(char ** buf, FILE * file, Htable ** tab, char oper, const char * cond_jmp, int * if_count, int * while_count, int if_cond, int while_cond);
void EMIT_PRINT(char ** buf, FILE * file);
void EMIT_INPUT(char ** buf, FILE * file);
void EMIT_EXIT(char ** buf);

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT    (char ** buf, FILE * file, int adr, const char * name);

#endif
