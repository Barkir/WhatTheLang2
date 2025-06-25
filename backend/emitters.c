#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "what_lang/constants.h"
#include "what_lang/nametable.h"
#include "what_lang/parser.h"
#include "what_lang/list.h"
#include "what_lang/htable.h"
#include "what_lang/tree.h"
#include "what_lang/backend.h"
#include "what_lang/backend_utils.h"
#include "what_lang/emitters.h"
#include "what_lang/errors.h"

#define EMIT(ctx, binary, command)                                  \
    DoEmit((ctx), (binary), (sizeof(binary) - 1), (command))

void DoEmit(BinCtx * ctx, const char * binary, size_t buf_len, const char * command)
{
    assert(ctx);
    assert(binary);
    assert(command);

    fprintf(ctx->file, "%s\n", command);
    memcpy(ctx->buf, binary, buf_len);
    ctx->buf += buf_len;
}

void EmitInt64(BinCtx * ctx, int64_t val)
{
    assert(ctx);

    memcpy(ctx->buf, &val, sizeof(int64_t));
    ctx->buf += sizeof(int64_t);
}

void EmitInt32(BinCtx * ctx, int val)
{
    assert(ctx);

    memcpy(ctx->buf, &val, sizeof(int));
    ctx->buf += sizeof(int);
}

void EmitByte(BinCtx * ctx, char byte)
{
    assert(ctx);

    *(ctx->buf) = byte;
    ctx->buf++;
}

void EmitPushImm32(BinCtx * ctx, field_t value)
{
    assert(ctx);

    int val = (int) value;
    fprintf(ctx->file, "push %d\n", val);
    EmitByte(ctx, PUSHIMM32_BYTE);
    EmitInt32(ctx, val);
}

void EmitPushReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    PARSER_LOG("PUSHING REG %d %s", reg, EnumReg2Str(reg, 0));
    fprintf(ctx->file, "push %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, PUSHREG_BYTE + reg);
}

void EmitPushXtendReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    PARSER_LOG("PUSHING REG %d %s", reg, EnumReg2Str(reg, 1));
    fprintf(ctx->file, "push %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, ADDITIONAL_REG_BYTE);
    EmitByte(ctx, PUSHREG_BYTE + reg);
}

void EmitPopReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    PARSER_LOG("POPPING REG %d %s", reg, EnumReg2Str(reg, 0));
    fprintf(ctx->file, "pop %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, POP_BYTE + reg);
}

void EmitPopXtendReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    PARSER_LOG("POPPING REG %d %s", reg, EnumReg2Str(reg, 1));
    fprintf(ctx->file, "pop %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, ADDITIONAL_REG_BYTE);
    EmitByte(ctx, POP_BYTE + reg);
}

void EmitMulReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    fprintf(ctx->file, "mul %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, MULDIV_START_BYTE);
    EmitByte(ctx, MULREG_BYTE + reg);
}

void EmitMulXtendReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    fprintf(ctx->file, "mul %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, XTEND_OPER_BYTE);
    EmitByte(ctx, MULDIV_START_BYTE);
    EmitByte(ctx, MULREG_BYTE + reg);
}

void EmitDivReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    EmitPushReg(ctx, WHAT_REG_EDX);                                     // Cleaning rdx to prevent
    EmitSubRegReg(ctx, WHAT_REG_EDX, WHAT_REG_EDX, WHAT_REG_REG);       // floating point error

    fprintf(ctx->file, "div %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, MULDIV_START_BYTE);
    EmitByte(ctx, DIVREG_BYTE + reg);

    EmitPopReg(ctx, WHAT_REG_EDX);                                      // restoring rdx
}

void EmitDivXtendReg(BinCtx * ctx, uint8_t reg)
{
    assert(ctx);

    EmitPushReg(ctx, WHAT_REG_EDX);                                     // Cleaning rdx to prevent
    EmitSubRegReg(ctx, WHAT_REG_EDX, WHAT_REG_EDX, WHAT_REG_REG);       // floating point error

    fprintf(ctx->file, "div %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, XTEND_OPER_BYTE);
    EmitByte(ctx, MULDIV_START_BYTE);
    EmitByte(ctx, DIVREG_BYTE + reg);

    EmitPopReg(ctx, WHAT_REG_EDX);                                      // restoring rdx
}

void EmitCmpRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
    assert(ctx);

    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, REG_REG_BYTE);
                                break;

        case WHAT_REG_XTEND:    fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, REG_XTEND_BYTE);
                                break;

        case WHAT_XTEND_REG:    fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, XTEND_REG_BYTE);
                                break;

        case WHAT_XTEND_XTEND:  fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, XTEND_XTEND_BYTE);
                                break;
    }

    EmitByte(ctx, CMP_REG_BYTE);
    uint8_t modrm = CalcModrmRegReg(reg1, reg2);
    EmitByte(ctx, modrm);
}

void EmitMovAbsXtend(BinCtx * ctx, uint8_t reg, int64_t val)
{
    assert(ctx);

    EmitByte(ctx, XTEND_REG_BYTE);
    EmitByte(ctx, MOV_REG_VAL_BYTE + reg);
    EmitInt64(ctx, val);
}

void EmitMovRegVal(BinCtx * ctx, uint8_t reg, int val, enum RegModes mode)
{
    assert(ctx);

    switch(mode)
    {
        case WHAT_REG_VAL:      fprintf(ctx->file, "mov %s, %d\n", EnumReg2Str(reg, 0), val);
                                EmitByte(ctx, MOV_REG_VAL_BYTE + reg);
                                break;

        case WHAT_XTEND_VAL:    fprintf(ctx->file, "mov %s, %d\n", EnumReg2Str(reg, 1), val);
                                EmitByte(ctx, ADDITIONAL_REG_BYTE);
                                EmitByte(ctx, MOV_REG_VAL_BYTE + reg);
                                break;
    }

    EmitInt32(ctx, val);
}

void EmitMovRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode, enum RegModes mem_mode)
{
    assert(ctx);

    switch(mode)
    {
        case WHAT_REG_REG:
                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s,   %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s,  [%s]\n",EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, REG_REG_BYTE);
                                break;

        case WHAT_REG_XTEND:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s,  %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s],%s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n",EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, REG_XTEND_BYTE);
                                break;

        case WHAT_XTEND_REG:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s,   %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s,  [%s]\n",EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, XTEND_REG_BYTE);
                                break;

        case WHAT_XTEND_XTEND:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s,   %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s,  [%s]\n",EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, XTEND_XTEND_BYTE);
                                break;
    }

    if (mem_mode == WHAT_NOMEM)
    {
        EmitByte(ctx, MOV_REG_BYTE);
        uint8_t modrm = CalcMovModrm(reg1, reg2, WHAT_NOMEM);
        EmitByte(ctx, modrm);
    }
    else if (mem_mode == WHAT_MEM1)
    {
        EmitByte(ctx, MOV_REG_BYTE);

        uint8_t modrm = CalcMovModrm(reg1, reg2, WHAT_MEM1);
        EmitByte(ctx, modrm);

        if ((reg1 & 7) == 4 || (reg1 & 7) == 5)
        {
            uint8_t sib = (0 << 6) | (4 << 3) | (reg1 & 7);
            EmitByte(ctx, sib);
        }
    }
    else if (mem_mode == WHAT_MEM2)
    {
        EmitByte(ctx, MOV_MEM_BYTE);

        uint8_t mod = 0b00000000;

        uint8_t modrm = CalcMovModrm(reg1, reg2, WHAT_MEM2);
        EmitByte(ctx, modrm);

        if ((reg2 & 7) == 4)
        {
            uint8_t sib = (reg2 & 7) | ((reg2 & 7) << 3);
            EmitByte(ctx, sib);
        }
    }

}

void EmitAddRegVal(BinCtx * ctx, uint8_t reg, int val, enum RegModes mode)
{
    assert(ctx);

    if (mode == WHAT_REG_VAL) {PARSER_LOG("ADDING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 0));}
    else                      {PARSER_LOG("ADDING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 1));}

    uint8_t fbyte = (mode == WHAT_REG_VAL) ? OPER_BYTE : XTEND_OPER_BYTE;

    if (fbyte == OPER_BYTE) fprintf(ctx->file, "add %s, %d\n", EnumReg2Str(reg, 0), val);
    else                    fprintf(ctx->file, "add %s, %d\n", EnumReg2Str(reg, 1), val);

    EmitByte(ctx, fbyte);

    if (reg == WHAT_REG_EAX)
        EmitByte(ctx, 0x5);

    else
    {
        EmitByte(ctx, ADD_REG_VAL_BYTE);

        uint8_t modrm = CalcModrmRegVal(reg, 0);
        EmitByte(ctx, modrm);
    }

    EmitInt32(ctx, val);
}

void EmitSubRegVal(BinCtx * ctx, uint8_t reg, int val, enum RegModes mode)
{
    assert(ctx);

    if (mode == WHAT_REG_VAL) {PARSER_LOG("SUBTRACTING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 0));}
    else                      {PARSER_LOG("SUBTRACTING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 1));}

    uint8_t fbyte = (mode == WHAT_REG_VAL) ? OPER_BYTE : XTEND_OPER_BYTE;

    if (fbyte == OPER_BYTE) fprintf(ctx->file, "sub %s, %d\n", EnumReg2Str(reg, 0), val);
    else                    fprintf(ctx->file, "sub %s, %d\n", EnumReg2Str(reg, 1), val);

    EmitByte(ctx, fbyte);

    if (reg == WHAT_REG_EAX)
        EmitByte(ctx, 0x2d);

    else
    {
        EmitByte(ctx, ADD_REG_VAL_BYTE);

        uint8_t modrm = CalcModrmRegVal(reg, 5);
        EmitByte(ctx, modrm);
    }

    EmitInt32(ctx, val);
}

void EmitAddRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
    assert(ctx);

    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, REG_REG_BYTE);
                                break;

        case WHAT_REG_XTEND:    fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, REG_XTEND_BYTE);
                                break;

        case WHAT_XTEND_REG:    fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, XTEND_REG_BYTE);
                                break;

        case WHAT_XTEND_XTEND:  fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, XTEND_XTEND_BYTE);
                                break;
    }

    EmitByte(ctx, ADD_REG_REG_BYTE);

    uint8_t modrm = CalcModrmRegReg(reg1, reg2);

    EmitByte(ctx, modrm);
}

void EmitSubRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
    assert(ctx);

    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                EmitByte(ctx, REG_REG_BYTE);
                                break;

        case WHAT_REG_XTEND:    fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                EmitByte(ctx, REG_XTEND_BYTE);
                                break;

        case WHAT_XTEND_REG:    fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                EmitByte(ctx, XTEND_REG_BYTE);
                                break;

        case WHAT_XTEND_XTEND:  fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                EmitByte(ctx, XTEND_XTEND_BYTE);
                                break;
    }

    EmitByte(ctx, SUB_REG_REG_BYTE);

    uint8_t modrm = CalcModrmRegReg(reg1, reg2);

    EmitByte(ctx, modrm);
}

void EmitJmp(BinCtx * ctx, char jmp, char offset)
{
    assert(ctx);

    EmitByte(ctx, jmp);
    EmitByte(ctx, offset);
}

void EmitLongJmp(BinCtx * ctx, int offset)
{
    assert(ctx);

    EmitByte(ctx, 0xe9);
    EmitInt32(ctx, offset);
}

void EmitBtm(BinCtx * ctx)
{
    assert(ctx);

    EmitMovRegVal(ctx, WHAT_REG_EAX, 0x3c, WHAT_REG_VAL);
    EMIT(ctx, "\x0f\x05", "syscall");
    fprintf(ctx->file, "%s", NASM_BTM);
}

void EmitTop(BinCtx * ctx)
{
    assert(ctx);

    fprintf(ctx->file, "%s", NASM_TOP);
    EmitMovAbsXtend(ctx, WHAT_REG_R13, BUF_R13_ADR);
    EmitMovAbsXtend(ctx, WHAT_REG_R12, BUF_R12_ADR);
}

void EmitPrint(BinCtx * ctx)
{
    assert(ctx);

    EmitPopReg(ctx, WHAT_REG_EAX);
    EmitByte(ctx, CALL_DIRECT_BYTE);
    fprintf(ctx->file, "call _IOLIB_OUTPUT   \n");

    int32_t adr = (ctx->buf_ptr + IOLIB_OFFSET + PRINT_CALL * 2) - (ctx->buf + sizeof(int32_t)); // sizeof(int32_t) needed for correct long jmp offset
    EmitInt32(ctx, adr);
}

void EmitInput(BinCtx * ctx)
{
    assert(ctx);

    fprintf(ctx->file, "call _IOLIB_INPUT    \n");
    EmitByte(ctx, CALL_DIRECT_BYTE);
    int32_t adr = (ctx->buf_ptr + IOLIB_OFFSET + INPUT_CALL * 2) - (ctx->buf + sizeof(int32_t)); // sizeof(int32_t) needed for correct long jmp offset
    EmitInt32(ctx, adr);
    EmitPushReg(ctx, WHAT_REG_EAX);
}

void EmitCallDirect(BinCtx * ctx, Node * root)
{
    assert(ctx);
    assert(root);

    PARSER_LOG("DIRECT_CALL of function with name %s...", NodeName(root));
    fprintf(ctx->file, "call %s\n", NodeName(root));
    EmitByte(ctx, CALL_DIRECT_BYTE);
    AddFuncAdr(ctx, root);
    EmitInt32(ctx, 0);
}

void EmitComparsionWhile(BinCtx * ctx, Htable ** tab, int nodeVal)
{
    assert(ctx);
    assert(tab);

    PARSER_LOG("EmitComparsionWhile");

    const char * cond_jmp = CmpStr (nodeVal);
    char oper             = CmpByte(nodeVal);

    Name  * locals = InitLocalName(LABEL_SIZE);
    assert(locals);

    EmitCmpRegsBlock(ctx);  // Comparing values, popping them to r14 and r15

    char * offset = EmitCondJmp(ctx, cond_jmp, "WHILE_SUB_COND", ctx->while_count, oper, 0); // >>>>>>> v
                                                                                             //         v
    EmitPushImm32(ctx, 0);                                                                   //         v
                                                                                             //         v
    EmitCondJmp         (ctx, NULL,   "jmp WHILE_FALSE", ctx->while_count, JMP_BYTE, 0);     //         v
    InsertLabelToHtable(ctx, tab, locals, "WHILE_FALSE", ctx->while_count);                  //         v
                                                                                             //         v
    InsertOffsetToPtr(ctx, offset, "WHILE_SUB_COND", ctx->while_count); // <<<<<<<<<<<<<<<<<<<<<<<<<<<< v

    EmitPushImm32(ctx, 1);

    EmitCondJmp        (ctx, NULL,    "jmp WHILE_TRUE", ctx->while_count, JMP_BYTE, 0);
    InsertLabelToHtable(ctx, tab, locals, "WHILE_TRUE", ctx->while_count);
}

void EmitComparsionIf(BinCtx * ctx, Htable ** tab, int nodeVal)
{
    assert(ctx);
    assert(tab);

    PARSER_LOG("EmitComparsionIf");

    const char * cond_jmp = CmpStr (nodeVal);
    char oper             = CmpByte(nodeVal);

    Name * locals = InitLocalName(LABEL_SIZE);
    assert(locals);

    EmitCmpRegsBlock(ctx);  // Comparing values, popping them to r14 and r15

    char * offset = EmitCondJmp(ctx, cond_jmp, "IF_SUB_COND", ctx->if_count, oper, 0); // >>>>>>>> v
                                                                                       //          v
    EmitPushImm32(ctx, 0);                                                             //          v
                                                                                       //          v
    EmitCondJmp(ctx, NULL, "jmp IF_END", ctx->if_count, JMP_BYTE, 0);                  //          v
    InsertLabelToHtable(ctx, tab, locals, "IF_END", ctx->if_count);                    //          v
                                                                                       //          v
    InsertOffsetToPtr(ctx, offset, "IF_SUB_COND", ctx->if_count); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<< v

    EmitPushImm32(ctx, 1);

    EmitCondJmp(ctx, NULL, "jmp IF", ctx->if_count, JMP_BYTE, 0);
    InsertLabelToHtable(ctx, tab, locals, "IF", ctx->if_count);
}

void EmitVar(BinCtx * ctx, Node * root)
{
    assert(ctx);
    assert(root);

    EmitPushXtendReg(ctx, WHAT_REG_R12);
    EmitAddRegVal   (ctx, WHAT_REG_R12, GetVarOffset(root, ctx) * 8, WHAT_XTEND_VAL);
    EmitMovRegReg   (ctx, Offset2EnumReg(GetVarOffset(root, ctx)), WHAT_REG_R12, WHAT_XTEND_REG, WHAT_MEM2);
    EmitPopXtendReg (ctx, WHAT_REG_R12);
    EmitPushReg     (ctx, Offset2EnumReg(GetVarOffset(root, ctx)));
}

void EmitNumParam(BinCtx * ctx, Node * root, Name ** param_array, int param)
{
    assert(ctx);
    assert(root);
    assert(param_array);

    EmitPushXtendReg(ctx, WHAT_REG_R12);
    EmitPushReg     (ctx, WHAT_REG_EAX);
    EmitAddRegVal   (ctx, WHAT_REG_R12, param_array[param]->stack_offset * 8, WHAT_XTEND_VAL);
    EmitMovRegVal   (ctx, WHAT_REG_EAX, (int) NodeValue(root), WHAT_REG_VAL);
    EmitMovRegReg   (ctx, WHAT_REG_R12, WHAT_REG_EAX, WHAT_XTEND_REG, WHAT_MEM1);
    EmitPopReg      (ctx, WHAT_REG_EAX);
    EmitPopXtendReg (ctx, WHAT_REG_R12);
    EmitPushImm32   (ctx, (int) NodeValue(root));
}

void EmitVarParam(BinCtx * ctx, Node * root, int param)
{
    assert(ctx);
    assert(root);

    EmitPushXtendReg    (ctx, WHAT_REG_R12);
    EmitAddRegVal       (ctx, WHAT_REG_R12, GetVarOffset(root, ctx) * 8, WHAT_XTEND_VAL);
    EmitMovRegReg       (ctx, Offset2EnumReg(param), WHAT_REG_R12, WHAT_XTEND_REG, WHAT_MEM2);
    EmitPopXtendReg     (ctx, WHAT_REG_R12);
    EmitPushReg         (ctx, Offset2EnumReg(param));
}

void EmitCmpRegsBlock(BinCtx * ctx)
{
    EmitPopXtendReg   (ctx, WHAT_REG_R15);                                      // Comparing two values from stack,
    EmitPopXtendReg   (ctx, WHAT_REG_R14);                                      // popping them to r14 and r15,
    EmitPushXtendReg  (ctx, WHAT_REG_R14);                                      //
    EmitPushXtendReg  (ctx, WHAT_REG_R15);                                      // saving r14 and r15
    EmitCmpRegReg     (ctx, WHAT_REG_R14, WHAT_REG_R15, WHAT_XTEND_XTEND);      // comparing r14, r15
}

void EmitComparsion(BinCtx * ctx, Htable ** tab, int nodeVal)
{
    assert(ctx);
    assert(tab);

    if      (ctx->if_cond)      EmitComparsionIf   (ctx, tab, nodeVal);
    else if (ctx->while_cond)   EmitComparsionWhile(ctx, tab, nodeVal);
}

void EmitRet(BinCtx * ctx)
{
    assert(ctx);

    EMIT(ctx, "\xc3", "ret");
}

void EmitFuncStackPush(BinCtx * ctx, Node * root)
{
    assert(ctx);
    assert(root);

    fprintf(ctx->file, "%s:\n", NodeName(root->left));

    EMIT(ctx, "\x41\x5e",           "pop r14"       );
    EMIT(ctx, "\x4d\x89\x75\0",     "mov [r13], r14");
    EMIT(ctx, "\x49\x83\xc5\x08",   "add r13, 8"    );
}

void EmitFuncStackRet(BinCtx * ctx, Node * root)
{
    assert(ctx);
    assert(root);

    EMIT(ctx, "\x49\x83\xed\x08",   "sub r13, 8"    );
    EMIT(ctx, "\x4d\x8b\x75\0",     "mov r14, [r13]");
    EMIT(ctx, "\x41\x56",           "push r14"      );
    EMIT(ctx, "\xc3",               "ret"           );
}

char * EmitCondJmp(BinCtx * ctx, const char * cond_jmp, const char * cond_str, int cond_count, uint8_t command_byte, char offset)
{
    assert(ctx);
    assert(cond_str);


    if (!cond_jmp) fprintf(ctx->file, "%s%d\n", cond_str, cond_count);
    else           fprintf(ctx->file, "%s %s%d\n", cond_jmp, cond_str, cond_count);
    EmitJmp(ctx, command_byte, offset);
    char * cond = ctx->buf - 1; // -1 is needed because jmp is calculated from its' 1st byte
    return cond;
}

uint8_t CalcModrmRegReg(uint8_t reg1, uint8_t reg2)
{
    uint8_t mod = 0b11;
    return (mod << 6) | (reg2 << 3) | reg1;
}

uint8_t CalcModrmRegVal(uint8_t reg, int term)
{
    return (0xc0) | (term << 3) | (reg & 7);
}

uint8_t CalcMovModrm(uint8_t reg1, uint8_t reg2, int mem_mode)
{
        uint8_t mod = 0;
        switch(mem_mode)
        {
            case WHAT_NOMEM:    mod = 0b11000000;
                                return mod | ((reg2 & 7) << 3) | (reg1 & 7);

            case WHAT_MEM1:     if ((reg1 & 7) == 4 || (reg1 & 7) == 5)
                                    return mod | ((reg2 & 7) << 3) | 0b100;
                                else
                                    return mod | ((reg2 & 7) << 3) | (reg1 & 7);

            case WHAT_MEM2:     if ((reg2 & 7) == 4)
                                    return mod | ((reg1 & 7) << 3) | 0b100;
                                else
                                    return mod | ((reg1 & 7) << 3) | (reg2 & 7);
        }
}

