#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "what_lang/errors.h"
#include "what_lang/nasm2elf.h"


int NASM2ELF(const char * filename)
{
    FILE * fp = fopen(filename, "r+");
    if (!fp) return WHAT_FILEOPEN_ERROR;


    size_t fsize = FileSize(fp);                                                            // getting file size
    char * buf = (char*) calloc(fsize + 1, 1);                                              // creating buf
    if (!buf)
    {
        perror("buffer allocation error");
        return WHAT_MEMALLOC_ERROR;
    }

    if (fread(buf, 1, fsize, fp) != fsize)
    {
        perror("file reading error");
        return WHAT_FILEWRITE_ERROR;                                                        // writing file to buf
    }


}

int LexicalSeparation(Token ** lexems, char * buf, size_t buf_size)
{
    if (!lexems)    return WHAT_NULLPOINTER_ERROR;
    if (!buf)       return WHAT_NULLPOINTER_ERROR;

    char * instr_pointer = buf;
    Token * lexems_pointer = *lexems;

    for (size_t i = 0; i < buf_size; i++)
    {
        if (buf[i] == ' ' || buf[i] == '\n')
        {
            (*lexems)->instr_txt = strndup(instr_pointer, (buf + i) - instr_pointer);       // duplication of lexem into lexem struct array
            (*lexems)->instr = GetInstr((*lexems)->instr_txt);
            (*lexems)++;                                                                    // incrementing pointer
            instr_pointer = buf + i;                                                        // moving instruction pointer
            while (*(instr_pointer) == ' ' || *(instr_pointer) == '\n') instr_pointer++;    // moving to nearest letter
        }
    }


    // Setting other values for our lexems




}

enum Instruction GetInstr(char * instr)
{
    if (!strcmp("push", instr))         return WHAT_PUSH;
    if (!strcmp("pop",  instr))         return WHAT_POP;
    if (!strcmp("mov",  instr))         return WHAT_MOV;
    if (!strcmp("add",  instr))         return WHAT_ADD;
    if (!strcmp("sub",  instr))         return WHAT_SUB;
    if (!strcmp("div",  instr))         return WHAT_DIV;
    if (!strcmp("mul",  instr))         return WHAT_MUL;
    if (!strcmp("call", instr))         return WHAT_CALL;

    if (!isdigit(*instr))               return WHAT_NUM;

}

size_t FileSize(FILE * fp)
{
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    return size;
}


