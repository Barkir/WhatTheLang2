#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <elf.h>
#include <assert.h>

#include "what_lang/constants.h"
#include "what_lang/errors.h"
#include "what_lang/nasm2elf.h"

void GenerateElfHeader(char ** buf)
{
    assert(buf);

    Elf64_Ehdr header = {};

    header.e_ident[EI_MAG0]         = ELFMAG0;                      // 0x7F
    header.e_ident[EI_MAG1]         = ELFMAG1;                      // 'E'
    header.e_ident[EI_MAG2]         = ELFMAG2;                      // 'L'
    header.e_ident[EI_MAG3]         = ELFMAG3;                      // 'F'
    header.e_ident[EI_CLASS]        = ELFCLASS64;                   // 64-bit
    header.e_ident[EI_DATA]         = ELFDATA2LSB;                  // little endian
    header.e_ident[EI_VERSION]      = EV_CURRENT;

    header.e_type                   = ET_EXEC;
    header.e_machine                = EM_X86_64;
    header.e_version                = EV_CURRENT;
    header.e_entry                  = ELF_ENTRY_POINT;              // entry point
    header.e_phoff                  = sizeof(Elf64_Ehdr);           // program header offset
    header.e_shoff                  = 0;                            // no section headers
    header.e_flags                  = 0;
    header.e_ehsize                 = sizeof(Elf64_Ehdr);
    header.e_phentsize              = sizeof(Elf64_Phdr);
    header.e_phnum                  = 2;                            // program and data section
    header.e_shentsize              = 0;
    header.e_shnum                  = 0;
    header.e_shstrndx               = SHN_UNDEF;

    Elf64_Phdr phdr_code = {};

    phdr_code.p_type                = PT_LOAD;
    phdr_code.p_offset              = 0x0;
    phdr_code.p_vaddr               = 0x400000;
    phdr_code.p_paddr               = 0x400000;
    phdr_code.p_filesz              = CODE_SEG_SIZE;
    phdr_code.p_memsz               = CODE_SEG_SIZE;
    phdr_code.p_flags               = PF_X;                         // execute mode
    phdr_code.p_align               = SEG_ALIGNMENT;

    Elf64_Phdr phdr_data = {};

    phdr_data.p_type                = PT_LOAD;
    phdr_data.p_offset              = DATA_SEG_OFFSET;
    phdr_data.p_vaddr               = 0x402000;
    phdr_data.p_paddr               = 0x402000;
    phdr_data.p_filesz              = DATA_SEG_SIZE;
    phdr_data.p_memsz               = DATA_SEG_SIZE;
    phdr_data.p_flags               = PF_R | PF_W;                  // read + write
    phdr_data.p_align               = SEG_ALIGNMENT;


    memcpy(*buf, &header, sizeof(Elf64_Ehdr));
    memcpy(*buf + sizeof(Elf64_Ehdr), &phdr_code, sizeof(Elf64_Phdr));
    memcpy(*buf + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr), &phdr_data, sizeof(Elf64_Phdr));
    (*buf) += BUF_OFFSET;
}

size_t FileSize(FILE * fp)
{
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    return size;
}


