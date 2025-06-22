#ifndef NASM2ELF_H
#define NASM2ELF_H

const static size_t BUF_OFFSET = 0x1000;

int NASM2ELF(const char * filename);
void GenerateElfHeader(char ** buf);
enum Instruction GetInstr(char * instr);
size_t FileSize(FILE * fp);

enum Instruction
{
    WHAT_DEFAULT = 0,
    WHAT_PUSH = 1,
    WHAT_POP = 2,
    WHAT_ADD = 3,
    WHAT_SUB = 4,
    WHAT_DIV = 5,
    WHAT_MUL = 6,
    WHAT_SQRT = 7,
    WHAT_SIN = 8,
    WHAT_COS = 9,
    WHAT_DUMP = 10,
    WHAT_IN = 11,
    WHAT_OUT = 12,
    WHAT_JMP = 13,
    WHAT_JA = 14,
    WHAT_JAE = 15,
    WHAT_JB = 16,
    WHAT_JBE = 17,
    WHAT_JE = 18,
    WHAT_JNE = 19,
    WHAT_LABEL = 20,
    WHAT_CALL = 21,
    WHAT_RET = 22,
    WHAT_HLT = 23,
    WHAT_EMPTY = 24,
    WHAT_SHOW = 25,
    WHAT_LOAD = 26,
    WHAT_MOV = 27,
    WHAT_NUM =  28
};

struct elf_hdr
{
    char e_ident[16];
    char e_type[2];
    char e_machine[2];
    char e_version[4];
    char e_entry[4];
    char e_phoff[4];
    char e_shoff[4];
    char e_flags[4];
    char e_ehsize[2];
    char e_phentsize[2];
    char e_phnum[2];
    char e_shentsize[2];
    char e_shnum[2];
    char e_shstrndx[2];
};

#endif
