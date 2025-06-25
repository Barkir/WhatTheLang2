#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "what_lang/constants.h"
#include "what_lang/buff.h"
#include "what_lang/tree.h"
#include "what_lang/errors.h"
#include "what_lang/parser.h"
#include "what_lang/tokenizer.h"

Tree * CreateTree(TreeInit init, TreeCmp cmp, TreeFree free)
{
    Tree * t = (Tree*) malloc(sizeof(Tree));
    if (!t) return NULL;
    *t = (Tree) {NULL, init, cmp, free};
    return t;
}

static int tree_create_node(Tree * t, Node ** node, const void * pair)
{
    assert(t);
    assert(node);
    assert(pair);

    if (!*node)
    {
        if (((*node) = (Node *) malloc(sizeof(Node))) == NULL) return 0;
        **node = (Node) {t->init ? t->init(pair) : pair, NULL, NULL};
        return 0;
    }

    if (t->cmp(pair, (*node)->value) > 0)
        return tree_create_node(t, &(*node)->right, pair);
    else
        return tree_create_node(t, &(*node)->left, pair);

    (*node)->left = (*node)->right = NULL;

    return 0;
}

int CreateNode(Tree * t, const void * pair)
{
    assert(t);
    assert(pair);

    return tree_create_node(t, &t->root, pair);
}

Node * _insert_tree(Tree * t, Node ** root, const void * pair)
{
    assert(t);
    assert(root);
    assert(pair);

    if (!*root)
    {
        if ((*root = (Node*) malloc(sizeof(Node))) == NULL) return NULL;
        (*root)->value = t->init ? t->init(pair) : pair;
        (*root)->right = NULL;
        (*root)->left = NULL;
        return *root;
    }

    if (t->cmp(pair, (*root)->value) > 0)
        return _insert_tree(t, &(*root)->right, pair);
    else
        return _insert_tree(t, &(*root)->left, pair);
}

int InsertTree(Tree * t, const void * pair)
{
    assert(t);
    assert(pair);

    return (_insert_tree(t, &t->root, pair) != NULL);
}

Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out)
{
    assert(tree);
    assert(node);
    assert(Out);
    if (!*node) return NULL;

    unsigned int color = NodeColor(*node);
    field_t field = NodeValue(*node);

    switch ((int) NodeType(*node))
    {
        case OPER:              PARSER_LOG("OPER");
                                PARSER_LOG("node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, _enum_to_name((int) field), *node, color);

                                fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, _enum_to_name((int) field), *node, color);
                                break;

        case VAR:               PARSER_LOG("VAR name = %s", NodeName(*node));
                                fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, NodeName(*node), *node, color);
                                break;

        case NUM:               PARSER_LOG("NUM = %d", (int) NodeValue(*node));
                                fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, NodeValue(*node), *node, color);
                                break;

        case FUNC_EXT:          PARSER_LOG("FUNC_EXT name = %s", NodeName(*node));
                                fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, _enum_to_name((int) field), *node, color);
                                break;

        case SEP_SYMB:          fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, (int) field, color);
                                break;

        case FUNC_INTER_DEF:    PARSER_LOG("FUNC_INTER_DEF name = %s", NodeName(*node));
                                fprintf(Out, "node%p [shape = Mrecord; label = \"{%s}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, NodeName(*node), color);
                                break;

        case FUNC_INTER_CALL:   PARSER_LOG("FUNC_INTER_CALL name = %s with color %x", NodeName(*node), color);
                                fprintf(Out, "node%p[shape = Mrecord; label = \"{%s}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, NodeName(*node), color);
                                break;

        default:                PARSER_LOG("DEFAULT");
                                fprintf(Out, "node%p [shape = Mrecord; label = \"{}\"; style = filled; fillcolor = \"#%06X\"];\n",
                                *node, color);
                                break;

    }


    if ((*node)->left)  fprintf(Out, "node%p -> node%p\n", *node, (*node)->left);
    if ((*node)->right) fprintf(Out, "node%p -> node%p\n", *node, (*node)->right);

    _tree_dump_func(tree, &(*node)->left, Out);
    _tree_dump_func(tree, &(*node)->right, Out);

    return tree;
}

Tree * TreeDump(Tree * tree, const char * FileName)
{
    assert(tree);
    assert(FileName);

    PARSER_LOG("Dumping Tree...");
    FILE * Out = fopen(FileName, "wb");

    fprintf(Out, "digraph\n{\n");
    _tree_dump_func(tree, &tree->root, Out);
    fprintf(Out, "}\n");

    char * command = malloc(DEF_SIZE);
    sprintf(command, "dot %s -T png -o %s.png", FileName, FileName);

    fclose(Out);

    system(command);
    free(command);

    return tree;
}

int TreeParse(Tree * tree, const char * filename)
{
    assert(tree);
    assert(filename);

    FILE * file = fopen(filename, "rb");
    if (!file) return FOPEN_ERROR;
    if (ferror(file)) return FOPEN_ERROR;

    char * expression = CreateBuf(file);
    if (!expression) return ALLOCATE_MEMORY_ERROR;

    const char * ptr = expression;
    int pointer = 0;

    Node ** array = StringTokenize(ptr, &pointer);

    PARSER_LOG("Node ** array = %p\n", (*array));

    pointer = 0;
    tree->root = GetMajor(array, &pointer);
    PARSER_LOG("Starting constant convolution...");

    tree->root = _constant_convolution(&tree->root);

    pointer = 0;
    while (array[pointer])
    {
        // free(array[pointer]->value);
        free(array[pointer]);
        pointer++;
    }

    free(array);

    if (fclose(file) == EOF) return FCLOSE_ERROR;
    free(expression);

    return 1;
}

void _destroy_tree(Tree * t, Node * n)
{
    assert(t);
    if (!n) return;

    PARSER_LOG("SUBTREE %p value = %lg (%c) %p. Destroying.", n, NodeValue(n), (int) NodeValue(n), ((Field*) n->value));

    _destroy_tree(t, n->left);
    _destroy_tree(t, n->right);

    if (t->free) t->free(n->value);
    PARSER_LOG("SUBTREE %p. Destroyed.", n);
    free(n);
}

    void DestroyTree(Tree * t)
{
    PARSER_LOG("STARTED TREE DESTROY");
    _destroy_tree(t, t->root);
    free(t);

}

field_t NodeValue(Node * node)
{
    assert(node);

    if (!node) return -1;
    // PARSER_LOG("Getting node value %p %lg(%c)", node, ((Field*)(node->value))->value, (int)((Field*)(node->value))->value);
    // return ((Field*)(node->value))->value;
    return ((Field*)((char*)node + sizeof(Node)))->value;
}

char * NodeName(Node * node)
{
    assert(node);

    if (!node) return NULL;
    // PARSER_LOG("Getting node name %p %s", node, ((Field*)(node->value))->name);
    // return ((Field*)(node->value))->name;
    return ((Field*)((char*)node + sizeof(Node)))->name;
}

int NodeIP(Node * node)
{
    assert(node);

    if (!node) return -1;
    return ((Field*)((char*)node + sizeof(Node)))->ip;
}

enum types NodeType(Node * node)
{
    assert(node);

    if (!node) return ERROR;
    // return ((Field*)(node->value))->type;
    return ((Field*)((char*)node + sizeof(Node)))->type;
}

const char * NodeType2Str(Node * node)
{
    assert(node);

    if (!node) return  "UNKNOWN_TYPE";

    switch (NodeType(node))
    {
        case OPER:              return "OPER"           ;
        case VAR :              return "VAR"            ;
        case NUM :              return "NUM"            ;
        case FUNC_EXT:          return "FUNC_EXT"       ;
        case SEP_SYMB:          return "SEP_SYMB"       ;
        case FUNC_INTER_DEF:    return "FUNC_INTER_DEF" ;
        case FUNC_INTER_CALL:   return "FUNC_INTER_CALL";
        default:                return "UNKNOWN TYPE"   ;
    }

    return "UNKNOWN_TYPE";
}

unsigned int NodeColor(Node * node)
{
    assert(node);

    unsigned int color = 0;
    switch (NodeType(node))
        {
        case OPER:
            color = OPER_COLOR;
            break;

        case VAR:
            color = VAR_COLOR;
            break;

        case NUM:
            color = NUM_COLOR;
            break;

        case FUNC_EXT:
            color = FUNC_EXT_COLOR;
            break;

        case SEP_SYMB:
            color = SEP_COLOR;
            break;

        case FUNC_INTER_DEF:
            color = FUNC_INTER_DEF_COLOR;
            break;

        case FUNC_INTER_CALL:
            color = FUNC_INTER_CALL_COLOR;
            break;

        default:
            color = SEP_COLOR;
            break;
        }

    return color;
}

Field * _create_field(field_t val, enum types type)
{
    Field * field = (Field*) calloc(1, sizeof(Field));
    if (!field) return NULL;

    field->value = val;
    field->type = type;

    return field;
}

Node * _create_node(Field * val, Node * left, Node * right)
{
    assert(val);

    PARSER_LOG("Creating node with field %lg, left %p right %p", val->value, left, right);
    Node * node = (Node*) calloc(1, sizeof(Node) + sizeof(Field));
    if (!node) return NULL;
    memcpy((char*)node + sizeof(Node), val, sizeof(Field));

    if (left) node->left = left;
    if (right) node->right = right;

    PARSER_LOG("Created node with field %lg, left %p, right %p", NodeValue(node));

    return node;
}

Field * _copy_field(Field * field)
{
    assert(field);

    if (!field) return NULL;
    Field * copy_field = (Field*) calloc(1, sizeof(Field));
    if (!copy_field) return NULL;

    copy_field->value = field->value;
    copy_field->type = field->type;

    return copy_field;
}

Node * _copy_node(Node * node)
{
    assert(node);

    if (!node) return NULL;
    PARSER_LOG("Copying node %p(left = %p, right = %p) with value %lg(%c) and name %s...", node, NodeValue(node), node->left, node->right, (int)(NodeValue(node)), NodeName(node));
    // Field * copy_field = _copy_field((Field*)node->value);

    // if (!copy_field) return NULL;
    Node * copy_node = (Node*) calloc(1, sizeof(Node) + sizeof(Field));
    if (!copy_node) return NULL;

    // copy_node->value = copy_field;

    memcpy(copy_node, node, sizeof(Node) + sizeof(Field));

    // memcpy(((Field*)copy_node->value)->name, ((Field*)node->value)->name, strlen(((Field*)node->value)->name));
    // PARSER_LOG("Created node with value %lg", copy_field->value);
    // if (node->left)  copy_node->left = node->left;
    // if (node->right) copy_node->right = node->right;

    PARSER_LOG("Copied node, value = %lg, name = %s, adr %p (left = %p, right = %p)", NodeValue(copy_node), NodeName(copy_node), copy_node, copy_node->left, copy_node->right);



    return copy_node;
}

Node * _constant_convolution(Node ** node)
{
    if (!*node)  return NULL;

    if (NodeType(*node) == OPER)
    {
        (*node)->left  = _constant_convolution(&((*node)->left));
        (*node)->right = _constant_convolution(&((*node)->right));

        if (NodeType((*node)->right) == NUM && NodeType((*node)->left) == NUM)
        {
            PARSER_LOG("CONSTANT CONVOLUTION");

            Field * field   = NULL;
            Node * new_node = NULL;
            field_t result = 0;

            switch((int) NodeValue((*node)))
            {

                case '+':   result = NodeValue((*node)->left) + NodeValue((*node)->right);
                            break;


                case '-':   result = NodeValue((*node)->left) - NodeValue((*node)->right);
                            break;


                case '*':   result = NodeValue((*node)->left) * NodeValue((*node)->right);
                            break;


                case '/':   assert(!NodeValue((*node)->right));
                            result = NodeValue((*node)->left) / NodeValue((*node)->right);
                            break;

            }

            Field result_field = {.value = result, .type = NUM};
            new_node = _create_node(&result_field, NULL, NULL);
            *node = new_node;
        }
    }

    _constant_convolution(&(*node)->left);
    _constant_convolution(&(*node)->right);

    return *node;
}

Node * _copy_branch(Node * node)
{
    if (!node) return NULL;
    PARSER_LOG("Copying branch %p with value %lg...", node, NodeValue(node));
    Node * result = _copy_node(node);
    if (!result) return NULL;

    result->left = _copy_branch(node->left);
    result->right = _copy_branch(node->right);

    return result;
}


