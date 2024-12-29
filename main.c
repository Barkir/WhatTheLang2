
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "buff.h"


int main(void)
{
    Tree * tree = CreateTree(NULL, NULL, NULL);
    TreeParse(tree, "toparse.txt");
    TreeDump(tree, "dump");
    CreateAsm(tree, "asm.asm");


    DestroyTree(tree);
}
