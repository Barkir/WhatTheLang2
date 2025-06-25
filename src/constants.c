#include <stdlib.h>

#include "what_lang/constants.h"

int IF_COUNT                            = 0;
int WHILE_COUNT                         = 0;
int ADR_COUNT                           = 0;
const int DEFAULT_REG_NUMBER            = 5;

// TODO: jmp-table for print and input functions

// trampoline table |
// jmp input        |
// jmp print        |

// call [syscall_table+0]|
// call [syscall_table+8]|

const int PRINT_OFFSET = 0x1527;
const int INPUT_OFFSET = 0x1500;

const unsigned int CRC32INIT            = 5381;

const size_t NUM_WORDS                  = 2000;
const size_t BUF_LEN                    = 32;
const size_t ALIGNED_SIZE               = 65536 * 128;

const size_t ELF_ENTRY_POINT =          0x401000;



const char * IOLIB_PATH                 = "iolib/iolib.o";
const char * GLOBAL_FUNC_NAME           = "GLOBAL_FUNC";


const char *     NASM_TOP               =  "%include \"iolib/iolib.asm\"        \n"
                                           "section .text                       \n"
                                           "global _start                       \n"
                                           "_start:                             \n"
                                           "mov r13, stack4calls                \n"
                                           "mov r12, array4var                  \n";

const char *     NASM_BTM               = "section .data                        \n"
                                          "stack4calls times 128 * 8 db 0       \n"
                                          "array4var   times 16 * 4  db 0       \n"
                                          "section .text                        \n";

const char * RET_PUSH_STR               =  "; pushing return address to stack   \n"
                                           "pop r14                             \n"
                                           "mov [r13], r14                      \n"
                                           "add r13, 8                          \n";

const char * RET_POP_STR                =   "; popping return address to stack  \n"
                                            "sub r13, 8                         \n"
                                            "mov r14, [r13]                     \n"
                                            "push r14                           \n"
                                            "ret                                \n";

const char * TREEDUMP_FNAME             =  "dumpshit/dump";
const char * HTABLE_NAMES_FNAME         = "dumpshit/htab_log_names.dmp" ;
const char * HTABLE_LOCALS_FNAME        = "dumpshit/htab_log_locals.dmp";

const int DEF_SIZE                      =     1024;
const int BUF_DEF_SIZE                  = 1024 * 12;


#define OPER_ARRAY_SIZE 6
#define REG_ARRAY_SIZE  16

