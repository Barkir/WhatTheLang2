#ifndef EMITTERS_H
#define EMITTERS_H

enum CommandBytes
{
    JMP_BYTE              = 0xeb,
    JA_BYTE               = 0x77,
    JAE_BYTE              = 0x73,
    JB_BYTE               = 0x72,
    JBE_BYTE              = 0x76,
    JE_BYTE               = 0x74,
    JNE_BYTE              = 0x75,
    ADD_REG_VAL_BYTE      = 0x81,
    ADD_REG_REG_BYTE      = 0x01,
    SUB_REG_REG_BYTE      = 0x29,
    VAL2REG_START         = 0xb8,
    PUSHREG_BYTE          = 0x50,
    PUSHIMM32_BYTE        = 0x68,
    POP_BYTE              = 0x58,
    CALL_DIRECT_BYTE      = 0xe8,
    ADDITIONAL_REG_BYTE   = 0x41,
    MULREG_BYTE           = 0xe0,
    DIVREG_BYTE           = 0xf0,
    OPER_BYTE             = 0x48,
    XTEND_OPER_BYTE       = 0x49,
    REG_REG_BYTE          = 0x48,
    XTEND_REG_BYTE        = 0x49,
    REG_XTEND_BYTE        = 0x4c,
    XTEND_XTEND_BYTE      = 0x4d,
    CMP_REG_BYTE          = 0x39,
    MOV_REG_BYTE          = 0x89,
    MOV_MEM_BYTE          = 0x8b,
    MOV_REG_VAL_BYTE      = 0xb8
};

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
void SUB_REG_VAL(char ** buf, uint8_t reg, int val,       enum RegModes mode, BinCtx * ctx);

void ADD_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void SUB_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx);
void MOV_REG_VAL(char ** buf, uint8_t reg,  int val,      enum RegModes mode, BinCtx * ctx);
void MOV_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, enum RegModes mem_mode, BinCtx * ctx);
void MOVABS_XTEND(char ** buf, uint8_t reg, int64_t val, BinCtx * ctx);

void EMIT_JMP(char ** buf, char jmp, char offset);
void EMIT_COMPARSION(char ** buf, Htable ** tab, int nodeVal, BinCtx * ctx);
void EMIT_PRINT(char ** buf, BinCtx * ctx);
void EMIT_INPUT(char ** buf, BinCtx * ctx);

void EMIT_LONG_JMP(char ** buf, int offset);


void EMIT_NASM_TOP(char ** buf, BinCtx * ctx);
void EMIT_NASM_BTM(char ** buf, BinCtx * ctx);

void EMIT_VAR(char ** buf, Node * root, BinCtx * ctx);
void EMIT_NUM_PARAM(char ** buf, Node * root, Name ** param_array, int param, BinCtx * ctx);
void EMIT_VAR_PARAM(char ** buf, Node * root, int param, BinCtx * ctx);

void DO_EMIT(char ** buf, const char * binary, size_t buf_len, const char * command, BinCtx * ctx);

void EMIT_FUNC_STACK_PUSH(char ** buf, Node * root, BinCtx * ctx);
void EMIT_FUNC_STACK_RET(char ** buf, Node * root, BinCtx * ctx);

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT    (char ** buf, Node * root, BinCtx * ctx);

#define USE_CMP                                                                 \
    POP_XTEND_REG   (buf,  WHAT_REG_R15, ctx);                                  \
    POP_XTEND_REG   (buf,  WHAT_REG_R14, ctx);                                  \
    PUSH_XTEND_REG  (buf,  WHAT_REG_R14, ctx);                                  \
    PUSH_XTEND_REG  (buf,  WHAT_REG_R15, ctx);                                  \
    CMP_REG_REG     (buf,  WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND, ctx)   \

#endif
