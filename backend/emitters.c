#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
    fprintf(ctx->file, "%s\n", command);
    for (int i = 0; i < buf_len; i++)
        EmitByte(ctx, binary[i]);
}

void EmitInt64(BinCtx * ctx, int64_t val)
{
    memcpy(ctx->buf, &val, sizeof(int64_t));
    ctx->buf += sizeof(int64_t);
}

void EmitInt32(BinCtx * ctx, int val)
{
    memcpy(ctx->buf, &val, sizeof(int));
    ctx->buf += sizeof(int);
}

void EmitByte(BinCtx * ctx, char byte)
{
    *(ctx->buf) = byte;
    ctx->buf++;
}

void EmitPushImm32(BinCtx * ctx, field_t value)
{
    int val = (int) value;
    fprintf(ctx->file, "push %d\n", val);
    EmitByte(ctx, PUSHIMM32_BYTE);
    EmitInt32(ctx, val);
}

void EmitPushReg(BinCtx * ctx, uint8_t reg)
{
    PARSER_LOG("PUSHING REG %d %s", reg, EnumReg2Str(reg, 0));
    fprintf(ctx->file, "push %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, PUSHREG_BYTE + reg);
}

void EmitPushXtendReg(BinCtx * ctx, uint8_t reg)
{
    PARSER_LOG("PUSHING REG %d %s", reg, EnumReg2Str(reg, 1));
    fprintf(ctx->file, "push %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, ADDITIONAL_REG_BYTE);
    EmitByte(ctx, PUSHREG_BYTE + reg);
}

void EmitPopReg(BinCtx * ctx, uint8_t reg)
{
    PARSER_LOG("POPPING REG %d %s", reg, EnumReg2Str(reg, 0));
    fprintf(ctx->file, "pop %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, POP_BYTE + reg);
}

void EmitPopXtendReg(BinCtx * ctx, uint8_t reg)
{
    PARSER_LOG("POPPING REG %d %s", reg, EnumReg2Str(reg, 1));
    fprintf(ctx->file, "pop %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, ADDITIONAL_REG_BYTE);
    EmitByte(ctx, POP_BYTE + reg);
}

void EmitMulReg(BinCtx * ctx, uint8_t reg)
{
    fprintf(ctx->file, "mul %s\n", EnumReg2Str(reg, 0));
    EmitByte(ctx, 0xf7);
    EmitByte(ctx, MULREG_BYTE + reg);
}

void EmitMulXtendReg(BinCtx * ctx, uint8_t reg)
{
    fprintf(ctx->file, "mul %s\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, XTEND_OPER_BYTE);
    EmitByte(ctx, 0xf7);
    EmitByte(ctx, MULREG_BYTE + reg);
}


void EmitDivReg(BinCtx * ctx, uint8_t reg)
{
    fprintf(ctx->file, "push rdx    \n");
    fprintf(ctx->file, "xor rdx, rdx\n");
    fprintf(ctx->file, "div %s      \n", EnumReg2Str(reg, 0));
    fprintf(ctx->file, "pop rdx     \n");

    EmitByte(ctx, 0xf7);
    EmitByte(ctx, DIVREG_BYTE + reg);
}

void EmitDivXtendReg(BinCtx * ctx, uint8_t reg)
{

    fprintf(ctx->file, "push rdx    \n");
    fprintf(ctx->file, "xor rdx, rdx\n");
    fprintf(ctx->file, "div %s      \n", EnumReg2Str(reg, 1));
    fprintf(ctx->file, "pop rdx     \n");

    EmitByte(ctx, XTEND_OPER_BYTE);
    EmitByte(ctx, 0xf7);
    EmitByte(ctx, DIVREG_BYTE + reg);
}

void EmitCmpRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
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

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    EmitByte(ctx, modrm);
}

void EmitMovAbsXtend(BinCtx * ctx, uint8_t reg, int64_t val)
{
    // fprintf(ctx->file, "movabs %s, %ld\n", EnumReg2Str(reg, 1));
    EmitByte(ctx, XTEND_REG_BYTE);
    EmitByte(ctx, MOV_REG_VAL_BYTE + reg);

    EmitInt64(ctx, val);
}

void EmitMovRegVal(BinCtx * ctx, uint8_t reg, int val, enum RegModes mode)
{
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
    switch(mode)
    {
        case WHAT_REG_REG:
                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, REG_REG_BYTE);
                                break;

        case WHAT_REG_XTEND:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, REG_XTEND_BYTE);
                                break;

        case WHAT_XTEND_REG:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                EmitByte(ctx, XTEND_REG_BYTE);
                                break;

        case WHAT_XTEND_XTEND:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                EmitByte(ctx, XTEND_XTEND_BYTE);
                                break;
    }

    if (mem_mode == WHAT_NOMEM)
    {
        EmitByte(ctx, MOV_REG_BYTE);

        uint8_t mod = 0b11000000;
        uint8_t modrm = mod | ((reg2 & 7) << 3) | (reg1 & 7);

        EmitByte(ctx, modrm);
    }
    else if (mem_mode == WHAT_MEM1)
    {
        EmitByte(ctx, MOV_REG_BYTE);

        uint8_t mod = 0b00000000;
        if ((reg1 & 7) == 4 || (reg1 & 7) == 5 || reg1 >= 8)
        {

            uint8_t modrm = mod | ((reg2 & 7) << 3) | 0b100;
            EmitByte(ctx, modrm);

            uint8_t sib = (0 << 6) | (4 << 3) | (reg1 & 7);
            EmitByte(ctx, sib);
        }
        else
        {
            uint8_t modrm = mod | ((reg2 & 7) << 3) | (reg1 & 7);
            EmitByte(ctx, modrm);
        }
    }
    else if (mem_mode == WHAT_MEM2)
    {
        EmitByte(ctx, MOV_MEM_BYTE);

        uint8_t mod = 0b00000000;
        if ((reg2 & 7) == 4)
        {
            uint8_t modrm = mod | ((reg1 & 7) << 3) | 0b100;
            EmitByte(ctx, modrm);

            uint8_t sib = (reg2 & 7) | ((reg2 & 7) << 3);
            EmitByte(ctx, sib);
        }
        else
        {
            uint8_t modrm = mod | ((reg1 & 7) << 3) | (reg2 & 7);
            EmitByte(ctx, modrm);
        }
    }

}

void EmitAddRegVal(BinCtx * ctx, uint8_t reg, int val, enum RegModes mode)
{

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
        uint8_t modrm = (0xc0) | (0 << 3) | (reg & 7);
        EmitByte(ctx, modrm);
    }

    EmitInt32(ctx, val);
}

void EmitSubRegVal(BinCtx * ctx, uint8_t reg, int val, enum RegModes mode)
{

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
        uint8_t modrm = (0xc0) | (5 << 3) | (reg & 7);
        EmitByte(ctx, modrm);
    }

    EmitInt32(ctx, val);
}

void EmitAddRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{
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

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    EmitByte(ctx, modrm);
}

void EmitSubRegReg(BinCtx * ctx, uint8_t reg1, uint8_t reg2, enum RegModes mode)
{

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

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    EmitByte(ctx, modrm);
}


void EmitJmp(BinCtx * ctx, char jmp, char offset)
{
    EmitByte(ctx, jmp);
    EmitByte(ctx, offset);
}

void EmitLongJmp(BinCtx * ctx, int offset)
{
    EmitByte(ctx, 0xe9);
    EmitInt32(ctx, offset);
}

void EmitBtm(BinCtx * ctx)
{
    EmitMovRegVal(ctx, WHAT_REG_EAX, 0x3c, WHAT_REG_VAL);
    EMIT(ctx, "\x0f\x05", "syscall");
    fprintf(ctx->file, "%s", NASM_BTM);
}

void EmitTop(BinCtx * ctx)
{
    fprintf(ctx->file, "%s", NASM_TOP);
    EmitMovAbsXtend(ctx, WHAT_REG_R13, 0x402000);
    EmitMovAbsXtend(ctx, WHAT_REG_R12, 0x402100);
}


void EmitPrint(BinCtx * ctx)
{
    EmitPopReg(ctx, WHAT_REG_EAX);
    EmitByte(ctx, CALL_DIRECT_BYTE);
    fprintf(ctx->file, "call _IOLIB_OUTPUT   \n");

    int32_t adr = (ctx->buf_ptr + PRINT_OFFSET) - (ctx->buf + 4);
    EmitInt32(ctx, adr);
}

void EmitInput(BinCtx * ctx)
{
    fprintf(ctx->file, "call _IOLIB_INPUT    \n");
    EmitByte(ctx, CALL_DIRECT_BYTE);
    int32_t adr = (ctx->buf_ptr + INPUT_OFFSET) - (ctx->buf + 4);
    EmitInt32(ctx, adr);
    EmitPushReg(ctx, WHAT_REG_EAX);


}

void CallDirect(BinCtx * ctx, Node * root)
{
    PARSER_LOG("DIRECT_CALL of function with name %s...", NodeName(root));
    fprintf(ctx->file, "call %s\n", NodeName(root));
    EmitByte(ctx, CALL_DIRECT_BYTE);
    AddFuncAdr(ctx, root);
    EmitInt32(ctx, 0);
}

void EmitComparsionWhile(BinCtx * ctx, Htable ** tab, int nodeVal)
{
    char * offset         = NULL;
    const char * cond_jmp = CmpStr (nodeVal);
    char oper             = CmpByte(nodeVal);

    Name locals = {};
    locals.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));

    USE_CMP;

    fprintf(ctx->file, "%s SUB_COND%d\n", cond_jmp, ctx->while_count);
    EmitJmp(ctx, oper, 0);

    offset = ctx->buf - 1;

    EmitPushImm32(ctx, 0);
    fprintf(ctx->file, "jmp WHILE_FALSE%d\n", ctx->while_count);
    EmitJmp(ctx, JMP_BYTE, 0);
    sprintf(locals.local_func_name, "WHILE_FALSE%d", ctx->while_count);
    locals.offset = ctx->buf - 1;
    HtableLabelInsert(tab, &locals);

    fprintf(ctx->file, "SUB_COND%d:\n", ctx->while_count);
    *offset = (int8_t)(ctx->buf - (offset + 1));

    EmitPushImm32(ctx, 1);

    fprintf(ctx->file, "jmp WHILE_TRUE%d\n", ctx->while_count);
    EmitJmp(ctx, JMP_BYTE, 0);

    sprintf(locals.local_func_name, "WHILE_TRUE%d", ctx->while_count);
    locals.offset = ctx->buf - 1;
    HtableLabelInsert(tab, &locals);
}

void EmitComparsionIf(BinCtx * ctx, Htable ** tab, int nodeVal)
{

    char * offset         = NULL;
    const char * cond_jmp = CmpStr (nodeVal);
    char oper             = CmpByte(nodeVal);

    Name locals = {};
    locals.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));

    USE_CMP;

    PARSER_LOG("if_count = %d, while_count = %d", ctx->if_count, ctx->while_count);

    fprintf(ctx->file, "%s SUB_COND%d\n", cond_jmp, ctx->if_count);
    EmitJmp(ctx, oper, 0);

    offset = ctx->buf - 1;

    EmitPushImm32(ctx, 0);

    fprintf(ctx->file, "jmp IF_END%d\n", ctx->if_count);
    EmitJmp(ctx, JMP_BYTE, 0);

    sprintf(locals.local_func_name, "IF_END%d", ctx->if_count);
    locals.offset = ctx->buf - 1;

    PARSER_LOG("Inserting local label %s with offset %p", locals.local_func_name, locals.offset);
    HtableLabelInsert(tab, &locals);


    fprintf(ctx->file, "SUB_COND%d:\n",    ctx->if_count);
    *offset = (int8_t)(ctx->buf - (offset + 1));

    EmitPushImm32(ctx, 1);


    PARSER_LOG("Inserting IF%d", ctx->if_count);
    fprintf(ctx->file, "jmp IF%d\n", ctx->if_count);
    EmitJmp(ctx, JMP_BYTE, 0);

    sprintf(locals.local_func_name, "IF%d", ctx->if_count);
    locals.offset = ctx->buf - 1;
    HtableLabelInsert(tab, &locals);
}

void EmitVar(BinCtx * ctx, Node * root)
{
    EmitPushXtendReg(ctx, WHAT_REG_R12);
    EmitAddRegVal   (ctx, WHAT_REG_R12, GetVarOffset(root, ctx) * 8, WHAT_XTEND_VAL);
    EmitMovRegReg   (ctx, Offset2EnumReg(GetVarOffset(root, ctx)), WHAT_REG_R12, WHAT_XTEND_REG, WHAT_MEM2);
    EmitPopXtendReg (ctx, WHAT_REG_R12);
    EmitPushReg     (ctx, Offset2EnumReg(GetVarOffset(root, ctx)));
}

void EmitNumParam(BinCtx * ctx, Node * root, Name ** param_array, int param)
{
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
    EmitPushXtendReg    (ctx, WHAT_REG_R12);
    EmitAddRegVal       (ctx, WHAT_REG_R12, GetVarOffset(root, ctx) * 8, WHAT_XTEND_VAL);
    EmitMovRegReg       (ctx, Offset2EnumReg(param), WHAT_REG_R12, WHAT_XTEND_REG, WHAT_MEM2);
    EmitPopXtendReg     (ctx, WHAT_REG_R12);
    EmitPushReg         (ctx, Offset2EnumReg(param));
}

void EmitComparsion(BinCtx * ctx, Htable ** tab, int nodeVal)
{
    if      (ctx->if_cond)      EmitComparsionIf   (ctx, tab, nodeVal);
    else if (ctx->while_cond)   EmitComparsionWhile(ctx, tab, nodeVal);
}

void EMIT_RET(BinCtx * ctx)
{
    EMIT(ctx, "\xc3", "ret");
}

void EmitFuncStackPush(BinCtx * ctx, Node * root)
{
    fprintf(ctx->file, "%s:\n", NodeName(root->left));

    EMIT(ctx, "\x41\x5e",           "pop r14"       );
    EMIT(ctx, "\x4d\x89\x75\0",     "mov [r13], r14");
    EMIT(ctx, "\x49\x83\xc5\x08",   "add r13, 8"    );
}


void EmitFuncStackRet(BinCtx * ctx, Node * root)
{
    EMIT(ctx, "\x49\x83\xed\x08",   "sub r13, 8"    );
    EMIT(ctx, "\x4d\x8b\x75\0",     "mov r14, [r13]");
    EMIT(ctx, "\x41\x56",           "push r14"      );
    EMIT(ctx, "\xc3",               "ret"           );
}

