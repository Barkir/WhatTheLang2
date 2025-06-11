#ifndef EMITTERS_H
#define EMITTERS_H


void PUSHIMM32      (char ** buf, field_t value, BinCtx * ctx);
void PUSHREG        (char ** buf, uint8_t reg, BinCtx * ctx);
void PUSH_XTEND_REG (char ** buf, uint8_t reg, BinCtx * ctx);
void POPREG         (char ** buf, uint8_t reg, BinCtx * ctx);
void POP_XTEND_REG  (char ** buf, uint8_t reg, BinCtx * ctx);
void MULREG         (char ** buf, uint8_t reg, BinCtx * ctx);
void MUL_XTEND_REG  (char ** buf, uint8_t reg, BinCtx * ctx);
void DIVREG         (char ** buf, uint8_t reg, BinCtx * ctx);
void DIV_XTEND_REG  (char ** buf, uint8_t reg, BinCtx * ctx);

void CMP_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void ADD_REG_VAL(char ** buf, uint8_t reg, field_t val,   enum RegModes mode, BinCtx * ctx);
void ADD_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void SUB_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void EMIT_JMP(char ** buf, char jmp, char offset);
void EMIT_COMPARSION(char ** buf, Htable ** tab, int nodeVal, BinCtx * ctx);
void EMIT_PRINT(char ** buf, BinCtx * ctx);
void EMIT_INPUT(char ** buf, BinCtx * ctx);
void EMIT_EXIT(char ** buf);

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT    (char ** buf, int adr, const char * name, BinCtx * ctx);

#endif
