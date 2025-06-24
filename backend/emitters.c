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

#define EMIT(buf, binary, command, ctx)                 \
    DO_EMIT((buf), (binary), (sizeof(binary) - 1), (command), (ctx))


void DO_EMIT(char ** buf, const char * binary, size_t buf_len, const char * command, BinCtx * ctx)
{
    fprintf(ctx->file, "%s\n", command);
    for (int i = 0; i < buf_len; i++)
    {
        **buf = binary[i];
        (*buf)++;
    }
}

void PUSHIMM32(char ** buf, field_t value, BinCtx * ctx)
{
    int val = (int) value;
    fprintf(ctx->file, "push %d\n", val);
    PARSER_LOG("PUSHING VALUE %d", val);
    **buf = PUSHIMM32_BYTE;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;
    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}

void PUSHREG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    PARSER_LOG("PUSHING REG %d %s", reg, EnumReg2Str(reg, 0));
    fprintf(ctx->file, "push %s\n", EnumReg2Str(reg, 0));
    **buf = PUSHREG_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", PUSHREG_BYTE, reg, **buf);
    (*buf)++;
}

void PUSH_XTEND_REG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    PARSER_LOG("PUSHING REG %d %s", reg, EnumReg2Str(reg, 1));
    fprintf(ctx->file, "push %s\n", EnumReg2Str(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;
    **buf = PUSHREG_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", PUSHREG_BYTE, reg, **buf);
    (*buf)++;
}

void POPREG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    PARSER_LOG("POPPING REG %d %s", reg, EnumReg2Str(reg, 0));
    fprintf(ctx->file, "pop %s\n", EnumReg2Str(reg, 0));
    (**buf) = POP_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", POP_BYTE, reg, **buf);
    (*buf)++;
}

void POP_XTEND_REG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    PARSER_LOG("POPPING REG %d %s", reg, EnumReg2Str(reg, 1));
    fprintf(ctx->file, "pop %s\n", EnumReg2Str(reg, 1));
    **buf = ADDITIONAL_REG_BYTE;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;
    **buf = POP_BYTE + reg;
    PARSER_LOG("OPCODE %x + %x = %x", POP_BYTE, reg, **buf);
    (*buf)++;
}

void MULREG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    fprintf(ctx->file, "mul %s\n", EnumReg2Str(reg, 0));
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}

void MUL_XTEND_REG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    fprintf(ctx->file, "mul %s\n", EnumReg2Str(reg, 1));
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = MULREG_BYTE + reg;
    (*buf)++;
}


void DIVREG(char ** buf, uint8_t reg, BinCtx * ctx)
{
    fprintf(ctx->file, "push rdx    \n");
    fprintf(ctx->file, "xor rdx, rdx\n");
    fprintf(ctx->file, "div %s      \n", EnumReg2Str(reg, 0));
    fprintf(ctx->file, "pop rdx     \n");
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void DIV_XTEND_REG(char ** buf, uint8_t reg, BinCtx * ctx)
{

    fprintf(ctx->file, "push rdx    \n");
    fprintf(ctx->file, "xor rdx, rdx\n");
    fprintf(ctx->file, "div %s      \n", EnumReg2Str(reg, 1));
    fprintf(ctx->file, "pop rdx     \n");
    **buf = XTEND_OPER_BYTE;
    (*buf)++;
    **buf = 0xf7;
    (*buf)++;
    **buf = DIVREG_BYTE + reg;
    (*buf)++;
}

void CMP_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx)
{
    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(ctx->file, "cmp %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                PARSER_LOG("COMPARING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = CMP_REG_BYTE;
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm;
    (*buf)++;

}

void MOVABS_XTEND(char ** buf, uint8_t reg, int64_t val, BinCtx * ctx)
{
    // fprintf(ctx->file, "movabs %s, %ld\n", EnumReg2Str(reg, 1));
    **buf = XTEND_REG_BYTE;
    (*buf)++;
    **buf = MOV_REG_VAL_BYTE + reg;
    (*buf)++;

    memcpy(*buf, &val, sizeof(int64_t));
    (*buf) += sizeof(int64_t);
}

void MOV_REG_VAL(char ** buf, uint8_t reg, int val, enum RegModes mode, BinCtx * ctx)
{
    switch(mode)
    {
        case WHAT_REG_VAL:      fprintf(ctx->file, "mov %s, %d\n", EnumReg2Str(reg, 0), val);
                                **buf = MOV_REG_VAL_BYTE + reg;
                                (*buf)++;
                                break;

        case WHAT_XTEND_VAL:    fprintf(ctx->file, "mov %s, %d\n", EnumReg2Str(reg, 1), val);
                                **buf = ADDITIONAL_REG_BYTE;
                                (*buf)++;
                                **buf = MOV_REG_VAL_BYTE + reg;
                                (*buf)++;
                                break;
    }

    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}

void MOV_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, enum RegModes mem_mode, BinCtx * ctx)
{

    switch(mode)
    {
        case WHAT_REG_REG:
                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:

                                if      (mem_mode == WHAT_NOMEM) fprintf(ctx->file, "mov %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM1)  fprintf(ctx->file, "mov [%s], %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                else if (mem_mode == WHAT_MEM2)  fprintf(ctx->file, "mov %s, [%s]\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));

                                PARSER_LOG("MOVING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    if (mem_mode == WHAT_NOMEM)
    {
        **buf = MOV_REG_BYTE;
        (*buf)++;

        uint8_t mod = 0b11000000;
        uint8_t modrm = mod | ((reg2 & 7) << 3) | (reg1 & 7);
        **buf = modrm;
        (*buf)++;
    }
    else if (mem_mode == WHAT_MEM1)
    {
        **buf = MOV_REG_BYTE;
        (*buf)++;

        uint8_t mod = 0b00000000;
        if ((reg1 & 7) == 4 || (reg1 & 7) == 5 || reg1 >= 8)
        {

            uint8_t modrm = mod | ((reg2 & 7) << 3) | 0b100;
            **buf = modrm;
            (*buf)++;

            uint8_t sib = (0 << 6) | (4 << 3) | (reg1 & 7);
            **buf = sib;
            (*buf)++;
        }
        else
        {
            uint8_t modrm = mod | ((reg2 & 7) << 3) | (reg1 & 7);
            **buf = modrm;
            (*buf)++;
        }
    }
    else if (mem_mode == WHAT_MEM2)
    {
        **buf = MOV_MEM_BYTE;
        (*buf)++;

        uint8_t mod = 0b00000000;
        if ((reg2 & 7) == 4) {
            uint8_t modrm = mod | ((reg1 & 7) << 3) | 0b100;
            **buf = modrm;
            (*buf)++;

            uint8_t sib = (reg2 & 7) | ((reg2 & 7) << 3);
            **buf = sib;
            (*buf)++;
        } else {
            uint8_t modrm = mod | ((reg1 & 7) << 3) | (reg2 & 7);
            **buf = modrm;
            (*buf)++;
        }
    }

}

void ADD_REG_VAL(char ** buf, uint8_t reg, int val, enum RegModes mode, BinCtx * ctx)
{

    if (mode == WHAT_REG_VAL) {PARSER_LOG("ADDING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 0));}
    else                      {PARSER_LOG("ADDING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 1));}

    uint8_t fbyte = (mode == WHAT_REG_VAL) ? OPER_BYTE : XTEND_OPER_BYTE;

    if (fbyte == OPER_BYTE) fprintf(ctx->file, "add %s, %d\n", EnumReg2Str(reg, 0), val);
    else                    fprintf(ctx->file, "add %s, %d\n", EnumReg2Str(reg, 1), val);

    **buf = fbyte;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;


    if (reg == WHAT_REG_EAX)
    {
        **buf = 0x5;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
    }

    else
    {
        uint8_t modrm = (0xc0) | (0 << 3) | (reg & 7);
        **buf = ADD_REG_VAL_BYTE;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
        **buf = modrm;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
    }

    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}

void SUB_REG_VAL(char ** buf, uint8_t reg, int val, enum RegModes mode, BinCtx * ctx)
{

    if (mode == WHAT_REG_VAL) {PARSER_LOG("SUBTRACTING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 0));}
    else                      {PARSER_LOG("SUBTRACTING VALUE %d to reg %d %s", val, reg, EnumReg2Str(reg, 1));}

    uint8_t fbyte = (mode == WHAT_REG_VAL) ? OPER_BYTE : XTEND_OPER_BYTE;

    if (fbyte == OPER_BYTE) fprintf(ctx->file, "sub %s, %d\n", EnumReg2Str(reg, 0), val);
    else                    fprintf(ctx->file, "sub %s, %d\n", EnumReg2Str(reg, 1), val);

    **buf = fbyte;
    PARSER_LOG("OPCODE %x", **buf);
    (*buf)++;


    if (reg == WHAT_REG_EAX)
    {
        **buf = 0x2d;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
    }

    else
    {
        uint8_t modrm = (0xc0) | (5 << 3) | (reg & 7);
        **buf = ADD_REG_VAL_BYTE;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
        **buf = modrm;
        PARSER_LOG("OPCODE %x", fbyte);
        (*buf)++;
    }

    memcpy(*buf, &val, sizeof(int));
    (*buf) += sizeof(int);
}

void ADD_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx)
{
    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 0), reg2, EnumReg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(ctx->file, "add %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                PARSER_LOG("ADDING REG %d %s to REG %d %s", reg1, EnumReg2Str(reg1, 1), reg2, EnumReg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = ADD_REG_REG_BYTE;
    PARSER_LOG("OPCODE %x", ADD_REG_REG_BYTE);
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm;
    (*buf)++;
}

void SUB_REG_REG(char ** buf, uint8_t reg1, uint8_t reg2, enum RegModes mode, BinCtx * ctx)
{

    switch(mode)
    {
        case WHAT_REG_REG:      fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 0));
                                **buf = REG_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_REG_XTEND:    fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 0), EnumReg2Str(reg2, 1));
                                **buf = REG_XTEND_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_REG:    fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 0));
                                **buf = XTEND_REG_BYTE;
                                (*buf)++;
                                break;

        case WHAT_XTEND_XTEND:  fprintf(ctx->file, "sub %s, %s\n", EnumReg2Str(reg1, 1), EnumReg2Str(reg2, 1));
                                **buf = XTEND_XTEND_BYTE;
                                (*buf)++;
                                break;
    }

    **buf = SUB_REG_REG_BYTE;
    (*buf)++;

    uint8_t mod = 0b11;
    uint8_t modrm = (mod << 6) | (reg2 << 3) | reg1;

    **buf = modrm;
    (*buf)++;
}


void EMIT_JMP(char ** buf, char jmp, char offset)
{
    **buf = jmp;
    PARSER_LOG("EMITTING JMP %x", **buf);
    (*buf)++;
    **buf = offset;
    PARSER_LOG("OFFSET %x", offset);
    (*buf)++;
}

void EMIT_LONG_JMP(char ** buf, int offset)
{

    PARSER_LOG("OFFSET %x, %d", offset, offset);
    **buf = 0xe9;
    (*buf)++;
    memcpy(*buf, &offset, sizeof(int));
    (*buf) += sizeof(int);
}

void EMIT_NASM_BTM(char ** buf, BinCtx * ctx)
{

    MOV_REG_VAL(buf, WHAT_REG_EAX, 0x3c, WHAT_REG_VAL, ctx);
    EMIT(buf, "\x0f\x05", "syscall", ctx);
    fprintf(ctx->file, "%s", NASM_BTM);
}

void EMIT_NASM_TOP(char ** buf, BinCtx * ctx)
{
    fprintf(ctx->file, "%s", NASM_TOP);
    MOVABS_XTEND(buf, WHAT_REG_R13, 0x402000, ctx);
    MOVABS_XTEND(buf, WHAT_REG_R12, 0x402100, ctx);
}


void EMIT_PRINT(char ** buf, BinCtx * ctx)
{
    PARSER_LOG("emitting print...");

    POPREG(buf, WHAT_REG_EAX, ctx);
    **buf = CALL_DIRECT_BYTE;
    (*buf)++;
    fprintf(ctx->file, "call _IOLIB_OUTPUT   \n");

    int32_t adr = (ctx->buf_ptr + 0x1527) - (*buf + 4);
    memcpy(*buf, &adr, sizeof(int32_t));
    (*buf) += sizeof(int32_t);

}

void EMIT_INPUT(char ** buf, BinCtx * ctx)
{
    PARSER_LOG("emitting input...");
    fprintf(ctx->file, "call _IOLIB_INPUT    \n");
    **buf = CALL_DIRECT_BYTE;
    (*buf)++;
    int32_t adr = (ctx->buf_ptr + 0x1500) - (*buf + 4);
    memcpy(*buf, &adr, sizeof(int32_t));
    (*buf) += sizeof(int32_t);

    PUSHREG(buf, WHAT_REG_EAX, ctx);


}

// ACHTUNG!!! WARNING!!! ACHTUNG!!!
// В CALL_DIRECT кладем абсолютный адрес

void CALL_DIRECT(char ** buf, Node * root, BinCtx * ctx)
{
    PARSER_LOG("DIRECT_CALL of function with name %s...", NodeName(root));
    fprintf(ctx->file, "call %s\n", NodeName(root));
    **buf = CALL_DIRECT_BYTE;
    (*buf)++;

    AddFuncAdr(buf, root, ctx);

    (*buf) += sizeof(int);
}

void EMIT_COMPARSION_WHILE(char ** buf, Htable ** tab, int nodeVal, BinCtx * ctx)
{
    char * offset         = NULL;
    const char * cond_jmp = CmpStr (nodeVal);
    char oper             = CmpByte(nodeVal);

    Name locals = {};
    locals.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));

    USE_CMP;

    fprintf(ctx->file, "%s SUB_COND%d\n", cond_jmp, ctx->while_count);
    EMIT_JMP(buf, oper, 0);

    offset = *buf - 1;

    PUSHIMM32(buf, 0, ctx);
    fprintf(ctx->file, "jmp WHILE_FALSE%d\n", ctx->while_count);
    EMIT_JMP(buf, JMP_BYTE, 0);
    sprintf(locals.local_func_name, "WHILE_FALSE%d", ctx->while_count);
    locals.offset = *buf - 1;
    HtableLabelInsert(tab, &locals);

    fprintf(ctx->file, "SUB_COND%d:\n", ctx->while_count);
    *offset = (int8_t)(*buf - (offset + 1));

    PUSHIMM32(buf, 1, ctx);

    fprintf(ctx->file, "jmp WHILE_TRUE%d\n", ctx->while_count);
    EMIT_JMP(buf, JMP_BYTE, 0);

    sprintf(locals.local_func_name, "WHILE_TRUE%d", ctx->while_count);
    locals.offset = *buf - 1;
    HtableLabelInsert(tab, &locals);
}

void EMIT_COMPARSION_IF(char ** buf, Htable ** tab, int nodeVal, BinCtx * ctx)
{

    char * offset         = NULL;
    const char * cond_jmp = CmpStr (nodeVal);
    char oper             = CmpByte(nodeVal);

    Name locals = {};
    locals.local_func_name = (char*) calloc(LABEL_SIZE, sizeof(char));

    USE_CMP;

    PARSER_LOG("if_count = %d, while_count = %d", ctx->if_count, ctx->while_count);

    fprintf(ctx->file, "%s SUB_COND%d\n", cond_jmp, ctx->if_count);
    EMIT_JMP(buf, oper, 0);

    offset = *buf - 1;

    PUSHIMM32(buf, 0, ctx);

    fprintf(ctx->file, "jmp IF_END%d\n", ctx->if_count);
    EMIT_JMP(buf, JMP_BYTE, 0);

    sprintf(locals.local_func_name, "IF_END%d", ctx->if_count);
    locals.offset = *buf - 1;

    PARSER_LOG("Inserting local label %s with offset %p", locals.local_func_name, locals.offset);
    HtableLabelInsert(tab, &locals);


    fprintf(ctx->file, "SUB_COND%d:\n",    ctx->if_count);
    *offset = (int8_t)(*buf - (offset + 1));

    PUSHIMM32(buf, 1, ctx);


    PARSER_LOG("Inserting IF%d", ctx->if_count);
    fprintf(ctx->file, "jmp IF%d\n", ctx->if_count);
    EMIT_JMP(buf, JMP_BYTE, 0);

    sprintf(locals.local_func_name, "IF%d", ctx->if_count);
    locals.offset = *buf - 1;
    HtableLabelInsert(tab, &locals);
}

void EMIT_VAR(char ** buf, Node * root, BinCtx * ctx)
{
    PUSH_XTEND_REG(buf, WHAT_REG_R12, ctx);
    ADD_REG_VAL(buf, WHAT_REG_R12, GetVarOffset(root, ctx) * 8, WHAT_XTEND_VAL, ctx);
    MOV_REG_REG(buf, Offset2EnumReg(GetVarOffset(root, ctx)), WHAT_REG_R12, WHAT_XTEND_REG, WHAT_MEM2, ctx);
    POP_XTEND_REG(buf, WHAT_REG_R12, ctx);
    PUSHREG(buf, Offset2EnumReg(GetVarOffset(root, ctx)), ctx);
}

void EMIT_NUM_PARAM(char ** buf, Node * root, Name ** param_array, int param, BinCtx * ctx)
{
    PUSH_XTEND_REG(buf, WHAT_REG_R12, ctx);
    PUSHREG(buf, WHAT_REG_EAX, ctx);
    ADD_REG_VAL(buf, WHAT_REG_R12, param_array[param]->stack_offset * 8, WHAT_XTEND_VAL, ctx);
    MOV_REG_VAL(buf, WHAT_REG_EAX, (int) NodeValue(root), WHAT_REG_VAL, ctx);
    MOV_REG_REG(buf, WHAT_REG_R12, WHAT_REG_EAX, WHAT_XTEND_REG, WHAT_MEM1, ctx);
    POPREG(buf, WHAT_REG_EAX, ctx);
    POP_XTEND_REG(buf, WHAT_REG_R12, ctx);
    PUSHIMM32(buf, (int) NodeValue(root), ctx);
}

void EMIT_VAR_PARAM(char ** buf, Node * root, int param, BinCtx * ctx)
{
    PUSH_XTEND_REG(buf, WHAT_REG_R12, ctx);
    ADD_REG_VAL(buf, WHAT_REG_R12, GetVarOffset(root, ctx) * 8, WHAT_XTEND_VAL, ctx);
    MOV_REG_REG(buf, Offset2EnumReg(param), WHAT_REG_R12, WHAT_XTEND_REG, WHAT_MEM2, ctx);
    POP_XTEND_REG(buf, WHAT_REG_R12, ctx);
    PUSHREG(buf, Offset2EnumReg(param), ctx);
}

void EMIT_COMPARSION(char ** buf, Htable ** tab, int nodeVal, BinCtx * ctx)
{
    if      (ctx->if_cond)      EMIT_COMPARSION_IF   (buf, tab, nodeVal, ctx);
    else if (ctx->while_cond)   EMIT_COMPARSION_WHILE(buf, tab, nodeVal, ctx);
}

void EMIT_RET(char ** buf, BinCtx * ctx)
{
    EMIT(buf, "\xc3", "ret", ctx);
}

void EMIT_FUNC_STACK_PUSH(char ** buf, Node * root, BinCtx * ctx)
{
    fprintf(ctx->file, "%s:\n", NodeName(root->left));

    EMIT(buf, "\x41\x5e",           "pop r14",        ctx);
    EMIT(buf, "\x4d\x89\x75\0",     "mov [r13], r14", ctx);
    EMIT(buf, "\x49\x83\xc5\x08",   "add r13, 8",     ctx);
}


void EMIT_FUNC_STACK_RET(char ** buf, Node * root, BinCtx * ctx)
{
    EMIT(buf, "\x49\x83\xed\x08",   "sub r13, 8",     ctx);
    EMIT(buf, "\x4d\x8b\x75\0",     "mov r14, [r13]", ctx);
    EMIT(buf, "\x41\x56",           "push r14",       ctx);
    EMIT(buf, "\xc3",               "ret",            ctx);
}

