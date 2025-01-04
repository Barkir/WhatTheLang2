
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "buff.h"


int main(void)
{
    Tree * tree = CreateTree(NULL, NULL, NULL);
    TreeParse(tree, "factorial.txt");
    TreeDump(tree, "dump");
    CreateAsm(tree, "asm.asm");
    system("Compiler/compiler/bin/Compiler asm.asm asm.out");
    system("Compiler/processor/bin/Processor asm.out");


    DestroyTree(tree);
}
