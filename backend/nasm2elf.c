#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <elf.h>

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
        return WHAT_FILEWRITE_ERROR;
    }


}

void GenerateElfHeader(char * buf)
{
    Elf64_Ehdr header = {};

    header.e_ident[EI_MAG0]         = ELFMAG0;                  // 0x7F
    header.e_ident[EI_MAG1]         = ELFMAG1;                  // 'E'
    header.e_ident[EI_MAG2]         = ELFMAG2;                  // 'L'
    header.e_ident[EI_MAG3]         = ELFMAG3;                  // 'F'
    header.e_ident[EI_CLASS]        = ELFCLASS64;               // 64-bit
    header.e_ident[EI_DATA]         = ELFDATA2LSB;              // little endian
    header.e_ident[EI_VERSION]      = EV_CURRENT;

    header.e_type                   = ET_EXEC;
    header.e_machine                = EM_X86_64;
    header.e_version                = EV_CURRENT;
    header.e_entry                  = 0x401000;                 // entry point
    header.e_phoff                  = sizeof(Elf64_Ehdr);       // program header offset
    header.e_shoff                  = 0;                        // no section headers
    header.e_flags                  = 0;
    header.e_ehsize                 = sizeof(Elf64_Ehdr);
    header.e_phentsize              = sizeof(Elf64_Phdr);
    header.e_phnum                  = 1;                        // one program header
    header.e_shentsize              = 0;
    header.e_shnum                  = 0;
    header.e_shstrndx               = SHN_UNDEF;

    Elf64_Phdr phdr = {};

    phdr.p_type                     = PT_LOAD;
    phdr.p_offset                   = 0x0;
    phdr.p_vaddr                    = 0x400000;
    phdr.p_paddr                    = 0x400000;
    phdr.p_filesz                   = 1024 * 8;
    phdr.p_memsz                    = 1024 * 8;
    phdr.p_flags                    = PF_R | PF_X;              // read + execute mode
    phdr.p_align                    = 0x1000;

    memcpy(buf, &header, sizeof(Elf64_Ehdr));
    memcpy(buf + sizeof(Elf64_Ehdr), &phdr, sizeof(Elf64_Phdr));
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


