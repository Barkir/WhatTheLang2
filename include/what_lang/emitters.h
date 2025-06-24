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

void EmitByte(BinCtx * ctx, char byte);
void EmitInt32(BinCtx * ctx, int val);
void EmitPushImm32      (BinCtx * ctx, field_t value);
void EmitPushReg        (BinCtx * ctx, uint8_t reg);
void EmitPushXtendReg   (BinCtx * ctx, uint8_t reg);
void EmitPopReg         (BinCtx * ctx, uint8_t reg);
void EmitPopXtendReg    (BinCtx * ctx, uint8_t reg);
void EmitMulReg         (BinCtx * ctx, uint8_t reg);
void EmitMulXtendReg    (BinCtx * ctx, uint8_t reg);
void EmitDivReg         (BinCtx * ctx, uint8_t reg);
void EmitDivXtendReg    (BinCtx * ctx, uint8_t reg);
void EmitCmpRegReg      (BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode);
void EmitAddRegVal      (BinCtx * ctx, uint8_t reg,  int val,      enum RegModes mode);
void EmitSubRegVal      (BinCtx * ctx, uint8_t reg,  int val,      enum RegModes mode);
void EmitAddRegReg      (BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode);
void EmitSubRegReg      (BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode);
void EmitMovRegVal      (BinCtx * ctx, uint8_t reg,  int val,      enum RegModes mode);
void EmitMovRegReg      (BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode, enum RegModes mem_mode);
void EmitMovAbsXtend    (BinCtx * ctx, uint8_t reg,  int64_t val);
void EmitJmp            (BinCtx * ctx, char jmp,     char offset);
void EmitComparsion     (BinCtx * ctx, Htable ** tab, int nodeVal);
void EmitPrint          (BinCtx * ctx);
void EmitInput          (BinCtx * ctx);
void EmitLongJmp        (BinCtx * ctx, int offset);
void EmitTop            (BinCtx * ctx);
void EmitBtm            (BinCtx * ctx);
void EmitVar            (BinCtx * ctx, Node * root);
void EmitNumParam       (BinCtx * ctx, Node * root, Name ** param_array, int param);
void EmitVarParam       (BinCtx * ctx, Node * root, int param);
void DoEmit             (BinCtx * ctx, const char * binary, size_t buf_len, const char * command);
void EmitFuncStackPush  (BinCtx * ctx, Node * root);
void EmitFuncStackRet   (BinCtx * ctx, Node * root);
void CallDirect         (BinCtx * ctx, Node * root);

#define USE_CMP                                                             \
    EmitPopXtendReg   (ctx, WHAT_REG_R15);                                  \
    EmitPopXtendReg   (ctx, WHAT_REG_R14);                                  \
    EmitPushXtendReg  (ctx, WHAT_REG_R14);                                  \
    EmitPushXtendReg  (ctx, WHAT_REG_R15);                                  \
    EmitCmpRegReg     (ctx, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND)

#endif
