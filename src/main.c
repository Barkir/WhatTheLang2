
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
        if (!strcmp(argv[1], "whatc"))
        {
            Tree * tree = CreateTree(NULL, NULL, NULL);
            if (TreeParse(tree, argv[2]) == FOPEN_ERROR) return -fprintf(stderr, "Can't open file... plz try again.\n");

            sprintf(ASM, "%s.asm", argv[2]);
            sprintf(run, "%s.out", argv[2]);

            CreateBin(tree, ASM, run, WHAT_DEBUG_MODE);

            TreeDump(tree, "dump");
            DestroyTree(tree);
        }
    }
    else return -fprintf(stderr, "Type: ./bin/WhatTheLang  whatc <filename>\n");
}
