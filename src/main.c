
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "what_lang/tree.h"
#include "what_lang/buff.h"
#include "what_lang/parser.h"
#include "what_lang/errors.h"

#include "what_lang/nametable.h"
#include "what_lang/list.h"
#include "what_lang/htable.h"
#include "what_lang/backend.h"

int main(int argc, char * argv[])
{
    char ASM[DEF_SIZE];
    char out[DEF_SIZE];
    char run[DEF_SIZE];

    if (argc == 3)
    {
        if (!strcmp(argv[1], "compile"))
        {
            Tree * tree = CreateTree(NULL, NULL, NULL);
            if (TreeParse(tree, argv[2]) == FOPEN_ERROR) return -fprintf(stderr, "Can't open file... plz try again.\n");
            TreeDump(tree, "dump");
            sprintf(ASM, "%s.asm", argv[2]);
            // CreateAsm(tree, ASM);
            CreateBin(tree, "hello");
            // DestroyTree(tree);

            sprintf(out, "SPU-Processor/compiler/bin/Compiler %s %s.out\n", ASM, argv[2]);
            system(out);

            return fprintf(stdout, "Created %s.out!\n", argv[2]);
        }

        if (!strcmp(argv[1], "run"))
        {
            sprintf(run, "SPU-Processor/processor/bin/Processor %s.out\n", argv[2]);
            system(run);
        }

            return 0;
    }
    else return -fprintf(stderr, "Type: ./bin/WhatTheLang  compile/run <filename>\n");
}
