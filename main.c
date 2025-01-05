
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "buff.h"


int main(int argc, char * argv[])
{
    if (argc == 2)
    {
            char out[DEF_SIZE];
            char run[DEF_SIZE];
            char ASM[DEF_SIZE];

            sprintf(ASM, "%s.asm", argv[1]);

            Tree * tree = CreateTree(NULL, NULL, NULL);
            TreeParse(tree, argv[1]);

            CreateAsm(tree, ASM);

            sprintf(out, "Compiler/compiler/bin/Compiler %s %s.out\n", ASM, argv[1]);
            sprintf(run, "Compiler/processor/bin/Processor %s.out\n", argv[1]);
            system(out);
            system(run);

            DestroyTree(tree);

            return 0;
    }
    else return -fprintf(stderr, "Type: ./what.out <filename>\n");
}
