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
void ADD_REG_VAL(char ** buf, uint8_t reg,  int val,      enum RegModes mode, BinCtx * ctx);
void ADD_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void SUB_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void MOV_REG_VAL(char ** buf, uint8_t reg,  int val,      enum RegModes mode, BinCtx * ctx);
void MOV_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, enum RegModes mem_mode, BinCtx * ctx);
void MOVABS_XTEND(char ** buf, uint8_t reg, int64_t val, BinCtx * ctx);

void EMIT_JMP(char ** buf, char jmp, char offset);
void EMIT_COMPARSION(char ** buf, Htable ** tab, int nodeVal, BinCtx * ctx);
void EMIT_PRINT(char ** buf, BinCtx * ctx);
void EMIT_INPUT(char ** buf, BinCtx * ctx);
void EMIT_EXIT(char ** buf);

void EMIT_LONG_JMP(char ** buf, int offset);

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT    (char ** buf, int adr, const char * name, BinCtx * ctx);

#define USE_CMP                                                                 \
    POP_XTEND_REG   (buf,  WHAT_REG_R15, ctx);                                  \
    POP_XTEND_REG   (buf,  WHAT_REG_R14, ctx);                                  \
    PUSH_XTEND_REG  (buf,  WHAT_REG_R14, ctx);                                  \
    PUSH_XTEND_REG  (buf,  WHAT_REG_R15, ctx);                                  \
    CMP_REG_REG     (buf,  WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND, ctx)   \

#endif
