#ifndef NASM2ELF_H
#define NASM2ELF_H

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
}


static struct elf_hdr Header =
{
    .e_ident        =   {   0x7f, 'E', 'L', 'F',
                        0x02, 0x01, 0x01, 0x00,
                        0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00      },
    .e_type         =   {   0x02, 0x00                  },
    .e_machine      =   {   0x3e, 0x00                  },
    .e_version      =   {   0x01, 0x00, 0x00, 0x00      },
    .e_entry        =   {   0x00, 0x10, 0x40, 0x00      },
    .e_phoff        =   {   0x34, 0x00, 0x00, 0x00      },
    .e_shoff        =   {   0x00, 0x00, 0x00, 0x00      },
    .e_flags        =   {   0x00, 0x00, 0x00, 0x00      },
    .e_ehsize       =   {   0x34, 0x00                  },
    .e_phnum        =   {   0x02, 0x00                  },
    .e_shentsize    =   {0x00, 0x00                     },
    .e_shnum        =   {0x00, 0x00                     },
    .e_shstrndx     =   {0x00, 0x00                     },

};

#endif
