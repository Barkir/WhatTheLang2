#ifndef EMIT_CONSTANTS_H
#define EMIT_CONSTANTS_H

const static char JMP_BYTE              = 0xeb;
const static char JA_BYTE               = 0x77;
const static char JAE_BYTE              = 0x73;
const static char JB_BYTE               = 0x72;
const static char JBE_BYTE              = 0x76;
const static char JE_BYTE               = 0x74;
const static char JNE_BYTE              = 0x75;
const static char ADD_REG_VAL_BYTE      = 0x81;
const static char ADD_REG_REG_BYTE      = 0x01;
const static char SUB_REG_REG_BYTE      = 0x29;
const static char VAL2REG_START         = 0xb8;
const static char PUSHREG_BYTE          = 0x50;
const static char PUSHIMM32_BYTE        = 0x68;
const static char POP_BYTE              = 0x59;
const static char CALL_DIRECT_BYTE      = 0xe8;
const static char ADDITIONAL_REG_BYTE   = 0x41;
const static char MULREG_BYTE           = 0xe0;
const static char DIVREG_BYTE           = 0xf0;
const static char OPER_BYTE             = 0x48;
const static char XTEND_OPER_BYTE       = 0x49;
const static char REG_REG_BYTE          = 0x48;
const static char XTEND_REG_BYTE        = 0x49;
const static char REG_XTEND_BYTE        = 0x4c;
const static char XTEND_XTEND_BYTE      = 0x4d;
const static char CMP_REG_BYTE          = 0x39;

#endif
