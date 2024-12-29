/*
 * MAJOR        := (OP';')+
 * OPERATOR     := IF';' | A';'
 * IF           := '('E')' A';'
 * ASSIGNMENT   := ID '=' E
 * ID           := [a-Z]+
 * EXPRESSION   := TERM ([+-] TERM)*
 * TERM         := FACT ([*\] FACT)*
 * FACT         := ATOM (^ FACT)?
 * ATOM         := NUMBER | '(' EXPR ')'
 * NUMBER       := '-' SIGN? INTEREG | FPL | FPR
 * INTEGER      := DIGIT+
 * FPL          := DIGIT+ FRACTION EXP?
 * FPR          := DIGIT* FRACTION DIGIT+ EXP?
 * DIGIT        := [0-9]
 * FRACTION     := '.' DIGIT*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "buff.h"
#include "parser.h"
#include "nametable.h"

#define DEBUG

#define SYNTAX_ERROR(exp, real)                     \
    {                                               \
        SyntaxError(exp, real, __func__, __LINE__); \
    }                                               \

#ifdef DEBUG
#define PARSER(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }
#else
#define PARSER(...)
#endif

typedef struct _node
{
    void * value;
    struct _node * left;
    struct _node * right;

} Node;

struct _tree
{
    Node * root;
    TreeInit init;
    TreeCmp cmp;
    TreeFree free;
};

static Field NameTable[] =
{
    {.type = FUNC,      .value = SIN},
    {.type = FUNC,      .value = COS},
    {.type = FUNC,      .value = TG},
    {.type = FUNC,      .value = CTG},
    {.type = FUNC,      .value = SH},
    {.type = FUNC,      .value = CH},
    {.type = FUNC,      .value = TH},
    {.type = FUNC,      .value = CTH},
    {.type = FUNC,      .value = LN},
    {.type = FUNC,      .value = LOG},
    {.type = OPER,      .value = IF},
    {.type = OPER,      .value = WHILE}
};


static int tree_create_node(Tree * t, Node ** node, const void * pair);
int _tree_parse(Tree* tree, Node ** node, const char ** string);
Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out);
Node * _insert_tree(Tree * t, Node ** root, const void * pair);
void _destroy_tree(Tree * t, Node * n);

const char * enum_to_name(int name);

int SyntaxError(char exp, char real, const char * func, int line);
Node ** StringTokenize(const char * string, int * p);

Node * GetMajor(Node ** nodes, int * p);
Node * GetOperator(Node ** nodes, int * p);
Node * GetAssignment(Node ** nodes, int * p);
Node * GetExpression(Node ** nodes, int * p);
Node * GetTerm(Node ** nodes, int * p);
Node * GetPow(Node ** nodes, int * p);
Node * GetBracket(Node ** nodes, int * p);
Node * GetFunc(Node ** nodes, int * p);
Node * GetNumber(Node ** nodes, int * p);
Node * GetID(Node ** nodes, int * p);
Node * GetIf(Node ** nodes, int * p);
Node * GetWhile(Node ** nodes, int * p);

Node * _copy_branch(Node * node);
Node * _copy_node(Node * node);
Field * _copy_field(Field * field);
Node * _create_node(Field * val, Node * left, Node * right);
Field * _create_field(field_t val, enum types type);

unsigned int NodeColor(Node * node);
field_t NodeValue(Node * node);
enum types NodeType(Node * node);
char * NodeName(Node * node);

static int IF_C = 0;
static int WHILE_C = 0;
static int ADR_C = 0;

Tree * CreateTree(TreeInit init, TreeCmp cmp, TreeFree free)
{
    Tree * t = (Tree*) malloc(sizeof(Tree));
    if (!t) return NULL;
    *t = (Tree) {NULL, init, cmp, free};
    return t;
}

static int tree_create_node(Tree * t, Node ** node, const void * pair)
{
    if (!*node)
    {
        if (((*node) = (Node *) malloc(sizeof(Node))) == NULL) return 0;
        **node = (Node) {t->init ? t->init(pair) : pair, NULL, NULL};
        return 0;
    }

    if (t->cmp(pair, (*node)->value) > 0)
        return _create_node(t, &(*node)->right, pair);
    else
        return _create_node(t, &(*node)->left, pair);

    (*node)->left = (*node)->right = NULL;

    return 0;
}

int CreateNode(Tree * t, const void * pair)
{
    return _create_node(t, &t->root, pair);
}

Node * _insert_tree(Tree * t, Node ** root, const void * pair)
{
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
    return !!_insert_tree(t, &t->root, pair);
}

Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out)
{

    if (!tree)
        return NULL;

    if (!*node) return NULL;

    unsigned int color = NodeColor(*node);
    field_t field = NodeValue(*node);

    switch (((Field*)(*node)->value)->type)
    {
        case OPER:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, enum_to_name((int) field), *node, color);
                    break;

        case VAR:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, NodeName(*node), *node, color);
                    break;

        case NUM:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, *node, field, color);
                    break;

        case FUNC:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, enum_to_name((int) field), *node, color);
                    break;

        case SEP_SYMB:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                        *node, (int) field, color);
                        break;

        default:    fprintf(Out, "node%p [shape = Mrecord; label = \"{}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, color);
                    break;

    }

    if ((*node)->left)
        fprintf(Out, "node%p -> node%p\n", *node, (*node)->left);

    if ((*node)->right)
        fprintf(Out, "node%p -> node%p\n", *node, (*node)->right);

    _tree_dump_func(tree, &(*node)->left, Out);
    _tree_dump_func(tree, &(*node)->right, Out);

    return tree;
}

Tree * TreeDump(Tree * tree, const char * FileName)
{
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
    FILE * file = fopen(filename, "rb");
    if (ferror(file)) return FOPEN_ERROR;

    char * expression = CreateBuf(file);
    if (!expression) return ALLOCATE_MEMORY_ERROR;

    const char * ptr = expression;
    int pointer = 0;

    Node ** array = StringTokenize(ptr, &pointer);

    PARSER("Node ** array = %p\n", (*array));

    pointer = 0;
    tree->root = GetMajor(array, &pointer);

    pointer = 0;
    while (array[pointer])
    {
        free(array[pointer]->value);
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
    if (!n) return;

    PARSER("SUBTREE %p value = %lg (%c) %p. Destroying.", n, NodeValue(n), (int) NodeValue(n), ((Field*) n->value));

    _destroy_tree(t, n->left);
    _destroy_tree(t, n->right);

    if (t->free) t->free(n->value);
    PARSER("SUBTREE %p. Destroyed.", n);
    free(n);
}

void DestroyTree(Tree * t)
{
    PARSER("STARTED TREE PARSER");
    _destroy_tree(t, t->root);
    free(t);

}

field_t NodeValue(Node * node)
{
    if (!node) return -1;
    PARSER("Getting node value %p %lg", node, ((Field*)(node->value))->value);
    return ((Field*)(node->value))->value;
}

char * NodeName(Node * node)
{
    if (!node) return NULL;
    PARSER("Getting node name %p %s", node, ((Field*)(node->value))->name);
    return ((Field*)(node->value))->name;
}

enum types NodeType(Node * node)
{
    if (!node) return ERROR;
    return ((Field*)(node->value))->type;
}

unsigned int NodeColor(Node * node)
{
    unsigned int color = 0;
    switch (((Field*) (node->value))->type)
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

        case FUNC:
            color = FUNC_COLOR;
            break;

        case SEP_SYMB:
            color = SEP_COLOR;
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
    Node * node = (Node*) calloc(1, sizeof(Node));
    if (!node) return NULL;
    node->value = val;

    if (left) node->left = left;
    if (right) node->right = right;
    return node;
}

Field * _copy_field(Field * field)
{
    if (!field) return NULL;
    Field * copy_field = (Field*) calloc(1, sizeof(Field));
    if (!copy_field) return NULL;

    copy_field->value = field->value;
    copy_field->type = field->type;

    return copy_field;
}

Node * _copy_node(Node * node)
{
    if (!node) return NULL;
    PARSER("Copying node %p with value %lg(%c)...", node, ((Field*)node->value)->value, (int)((Field*)node->value)->value);
    Field * copy_field = _copy_field((Field*)node->value);
    if (!copy_field) return NULL;
    Node * copy_node = (Node*) calloc(1, sizeof(Node));
    if (!copy_node) return NULL;

    copy_node->value = copy_field;
    memcpy(((Field*)copy_node->value)->name, ((Field*)node->value)->name, strlen(((Field*)node->value)->name));
    PARSER("Created node with value %lg", copy_field->value);
    if (node->left)  copy_node->left = node->left;
    if (node->right) copy_node->right = node->right;

    return copy_node;
}

Node * _copy_branch(Node * node)
{
    if (!node) return NULL;
    PARSER("Copying branch %p with value %lg...", node, ((Field*)node->value)->value);
    Node * result = _copy_node(node);
    if (!result) return NULL;

    result->left = _copy_branch(node->left);
    result->right = _copy_branch(node->right);

    return result;
}

#define SKIPSPACE while(isspace(string[*p])) (*p)++;

int SyntaxError(char exp, char real, const char * func, int line)
{
    fprintf(stderr, ">>> SyntaxError %s %d: <expected %c> <got %c>", func, line, exp, real);
    assert(0);
}

Field _name_table(int name)
{
    Field ret = {.type = NUM, .value = -1};
    if (name < 0) return ret;
    for (int i = 0; i < DEF_SIZE; i++)
        if (NameTable[i].value == (field_t) name) return NameTable[i];
    return ret;
}

int _name_to_enum(char * name)
{
    if (strcmp(name, "sin") == 0)       return SIN;
    if (strcmp(name, "cos") == 0)       return COS;
    if (strcmp(name, "tg") == 0)        return TG;
    if (strcmp(name, "ctg") == 0)       return CTG;
    if (strcmp(name, "sh") == 0)        return SH;
    if (strcmp(name, "ch") == 0)        return CH;
    if (strcmp(name, "ln") == 0)        return LN;
    if (strcmp(name, "log") == 0)       return LOG;
    if (strcmp(name, "if") == 0)        return IF;
    if (strcmp(name, "while") == 0)     return WHILE;

    return -1;
}

const char * enum_to_name(int name)
{
    if (name == SIN) return "sin";
    if (name == COS) return "cos";
    if (name == SH)  return "sh";
    if (name == CH) return "ch";
    if (name == TG)  return "tg";
    if (name == LOG) return "log";
    if (name == CTG) return "ctg";
    if (name == LN)  return "ln";
    if (name == TH)     return "th";
    if (name == CTH)    return "cth";
    if (name == IF)     return "if";
    if (name == WHILE)  return "while";
    if (name == '+')    return "+";
    if (name == '=')    return "=";
    if (name == '/')    return "//";
    if (name == '*')    return "*";
    if (name == '^')    return "^";
    if (name == '-')    return "-";

    return "notfound";
}

Node * _find_name(char * result)
{
    PARSER("Need to find name %s... ", result);
    Field name = _name_table(_name_to_enum(result));

    Field * field = NULL;

    if (name.value < 0)
    {
        field = calloc(1, sizeof(Field));
        memcpy(field->name, result, strlen(result));
        field->type = VAR;
        field->value = 0;
    }

    else field = _create_field(name.value, name.type);
    if (!field) return NULL;
    Node * node = _create_node(field, NULL, NULL);
    if (!node) return NULL;
    return node;
}

Node * _oper_token(const char * string, int * p)
{
    SKIPSPACE

    Field * field = _create_field((field_t)string[*p], OPER);
    if  (!field) return NULL;
    Node * result = _create_node(field, NULL, NULL);
    if (!result) return NULL;
    (*p)++;
    return result;
}

Node * _name_token(const char * string, int * p)
{
    SKIPSPACE
    int start_p = *p;

    while(isalpha(string[*p])) (*p)++;

    const char * result = (const char*) calloc((*p) - start_p + 1, 1);
    memcpy(result, &string[start_p], (*p) - start_p);

    Node * name = _find_name(result);
    free(result);
    if (!name) return NULL;
    return name;
}

Node * _number_token(const char * string, int * p)
{
    SKIPSPACE
    char * end = NULL;
    field_t number = strtod(&(string[*p]), &end);
    if (!end) return NULL;
    (*p) += (int)(end - &string[*p]);
    Field * field = _create_field(number, NUM);
    if (!field) return NULL;
    Node * num = _create_node(field, NULL, NULL);
    if (!num) return NULL;
    return num;
}

Node * _sep_token(const char * string, int * p)
{
    SKIPSPACE
    Field * field = _create_field((field_t) ';', SEP_SYMB);
    if (!field) return NULL;
    Node * num = _create_node(field, NULL, NULL);
    if (!num) return NULL;
    (*p)++;
    return num;
}

Node * _get_token(const char * string, int * p)
{
    SKIPSPACE
    if (string[*p] == '(' ||
        string[*p] == ')' ||
        string[*p] == '+' ||
        string[*p] == '-' ||
        string[*p] == '/' ||
        string[*p] == '*' ||
        string[*p] == '^' ||
        string[*p] == '=')
        {PARSER("operator %c! ", string[*p]); return _oper_token(string, p);}

    if (string[*p] == ';') {PARSER("separator of line!"); return _sep_token(string, p);}

    if (isalpha(string[*p])) {PARSER("name %c! ", string[*p]); return _name_token(string, p);}

    if (isdigit(string[*p])) {PARSER("number %c! ", string[*p]); return _number_token(string, p);}

}

Node ** StringTokenize(const char * string, int * p)
{
    SKIPSPACE
    int size = 0;
    size_t arr_size = DEF_SIZE;
    Node ** nodes = (Node**) calloc(arr_size, sizeof(Node*));
    while (string[*p] != 0)
    {
        *(nodes + size) = _get_token(string, p);
        PARSER("got node %u %p with value %lg!\n", size+1, *(nodes + size), NodeValue(*(nodes + size)));
        size++;
    }
    return nodes;
}

Node * GetMajor(Node ** nodes, int * p)
{
    PARSER("Got node %p", nodes[*p]);
    Node * result = NULL;
    Node * operation = NULL;
    Field * oper = NULL;
    operation = GetOperator(nodes, p);
    while (operation)
    {
        oper = _create_field(';', SEP_SYMB);
        result = _create_node(oper, result, operation);
        operation = GetOperator(nodes, p);
    }
    PARSER("Got result!");
    if (!nodes[*p]) return result;
    PARSER("PARSING ENDED");
    (*p)++;
    return result;
}

Node * GetOperator(Node ** nodes, int * p)
{
    int old_p = (*p);
    if (!nodes[*p]) return NULL;
    PARSER("Getting O... Got node %p", nodes[*p]);
    Node * val1 = NULL;
    if (val1 = GetAssignment(nodes, p))
    {
        PARSER("Got Assignment...");
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER("Got ';'");
            (*p)++;
            return val1;
        }
        SYNTAX_ERROR(';', (int) NodeValue(nodes[*p]));
    }
    if ((val1 = GetIf(nodes, p)))
    {

        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER("Got ';'");
            (*p)++;
            return val1;
        }
        SYNTAX_ERROR(';', (int) NodeValue(nodes[*p]));
    }
    if (val1 = GetWhile(nodes, p))
    {
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER("Got ';'");
            (*p)++;
            return val1;
        }
        SYNTAX_ERROR(';', (int) NodeValue(nodes[*p]));
    }
    (*p) = old_p;
    return NULL;
}

Node * GetIf(Node ** nodes, int * p)
{
    if ((int) NodeValue(nodes[*p]) != IF) return NULL;
    PARSER("Getting IF");
    int old_p = (*p);
    Node * E = NULL;
    Node * A = NULL;

    Field * oper = _create_field((field_t) IF, OPER);
    (*p)++;

    if ((E = GetBracket(nodes, p)) && (A = GetAssignment(nodes, p)))
        return _create_node(oper, E, A);

    (*p) = old_p;
    return NULL;
}

Node * GetWhile(Node ** nodes, int * p)
{
    PARSER("Getting WHILE");
    int old_p = (*p);

    Node * E = NULL;
    Node * A = NULL;

    Field * oper = _create_field((field_t) WHILE, OPER);
    (*p)++;

    if (((int) NodeValue(nodes[(*p)++]) == '(') && (E = GetExpression(nodes, p)) && ((int) NodeValue(nodes[(*p)++]) == ')') && (A = GetAssignment(nodes, p)))
        return _create_node(oper, E, A);

    (*p) = old_p;
    return NULL;
}

Node * GetAssignment(Node ** nodes, int * p)
{
    int old_p = (*p);
    if (!nodes[*p]) return NULL;
    PARSER("Getting A... Got node %p %d", nodes[*p], NodeType(nodes[*p]));
    if (NodeType(nodes[*p]) != VAR) return NULL;
    Node * val1 = GetID(nodes, p);
    if (!val1) return  NULL;
    PARSER("Got ID node %p", val1);
    if ((int)NodeValue(nodes[*p]) == '=')
    {
        Field * operation = _create_field((field_t) '=', OPER);
        (*p)++;
        Node * val2 = GetExpression(nodes, p);
        if (!val2)
        {
            (*p) = old_p;
            return NULL;
        }
        val1 = _create_node(operation, val1, val2);
        return val1;
    }
    (*p) = old_p;
    return NULL;

}

Node * GetExpression(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting E... Got node %p", nodes[*p]);
    Node * val1 = GetTerm(nodes, p);
    if (!nodes[*p]) return val1;

    while ((int)NodeValue(nodes[*p]) == '+' || (int)NodeValue(nodes[*p]) == '-')
    {
        PARSER("Got node %p", nodes[*p]);
        int op = (int) NodeValue(nodes[*p]);

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        (*p)++;
        if (!nodes[*p]) return NULL;

        Node * val2 = GetTerm(nodes, p);
        PARSER("Got val2");

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetExpression Finished");
    return val1;
}

Node * GetTerm(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting T... Got node %p", nodes[*p]);
    PARSER("Getting pow in T val1..."); Node * val1 = GetPow(nodes, p);
    if (!nodes[*p]) {PARSER("GetTerm Finished!"); return val1;}

    while ((int)NodeValue(nodes[*p]) == '*' || (int)NodeValue(nodes[*p]) == '/')
    {
        int op = (int)NodeValue(nodes[*p]);

        Field * operation = NULL;

        if (op == '*') operation = _create_field((field_t) op, OPER);
        else if (op == '/') operation = _create_field((field_t) op, OPER);

        if (!operation) return NULL;


        (*p)++;
        if (!nodes[*p]) return val1;

        PARSER("Getting pow in T val2..."); Node * val2 = GetPow(nodes, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetTerm Finished");
    return val1;
}

Node * GetPow(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting pow... Got node %p", nodes[*p]);
    Node * val1 = GetBracket(nodes, p);
    if (!nodes[*p]) {PARSER("GetPow Finished"); return val1;}
    while ((int)NodeValue(nodes[(*p)]) == '^')
    {
        PARSER("Got '^'");
        int op = (int) NodeValue(nodes[(*p)]);

        Field * operation = NULL;

        (*p)++;
        if (!nodes[*p]) return val1;

        Node * val2 = GetBracket(nodes, p);
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetPow Finished");
    return val1;
}


Node * GetBracket(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("node = %p. Getting P...", nodes[*p]);
    if ((int) NodeValue(nodes[*p]) == '(')
    {
        (*p)++;
        PARSER("Got '('");
        Node * val = GetExpression(nodes, p);
        PARSER("Got node %p", nodes[*p]);
        if (!nodes[*p]) return val;
        if ((int) NodeValue(nodes[(*p)]) != ')') SYNTAX_ERROR(')', (int) NodeValue(nodes[(*p)]));
        PARSER("Got ')'");
        (*p)++;
        return val;
    }

    else if (NodeType(nodes[*p]) == FUNC) return GetFunc(nodes, p);
    else if (NodeType(nodes[*p]) == VAR) return GetID(nodes, p);
    else return GetNumber(nodes, p);
}

Node * GetFunc(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;

    Node * val = GetBracket(nodes, p);
    if (!val) return NULL;
    result->left = val;
    return result;
}

Node * GetNumber(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

Node * GetID(Node ** nodes, int * p)
{
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

const char * GetVarName(Node * root)
{
    switch((int) NodeValue(root))
    {
        case 'a': return "AX";
        case 'b': return "BX";
        case 'c': return "CX";
        case 'x': return "DX";
        case 'y': return "FX";
        default : return "RX";
    }
    return "RX";
}

int GetVarAdr(Node * root, Name * names)
{
    for(int i = 0; i < 1024; i++)
    {
        if (!(names[i].name)) return -1;
        if (!strcmp(NodeName(root), names[i].name)) return names[i].address;
    }

    return -1;
}

int _create_asm(Name * names, Node * root, FILE * file, int if_count, int while_count)
{
    if (NodeType(root) == NUM) fprintf(file, "push %lg\n", NodeValue(root));
    if (NodeType(root) == VAR) fprintf(file, "push [%d]\n", GetVarAdr(root, names));

    if (NodeType(root) == OPER)
    {
        switch ((int) NodeValue(root))
        {
            case '=':   _create_asm(names, root->right, file, IF_C, WHILE_C);
                        fprintf(file, "pop [%d]\n", GetVarAdr(root->left, names));
                        break;

            case '+':   _create_asm(names, root->left, file, IF_C, WHILE_C);
                        _create_asm(names, root->right, file, IF_C, WHILE_C);
                        fprintf(file, "add\n");
                        break;

            case '-':   _create_asm(names, root->left, file, IF_C, WHILE_C);
                        _create_asm(names, root->right, file, IF_C, WHILE_C);
                        fprintf(file, "sub\n");
                        break;

            case '*':   _create_asm(names, root->left, file, IF_C, WHILE_C);
                        _create_asm(names, root->right, file, IF_C, WHILE_C);
                        fprintf(file, "mul\n");
                        break;

            case '/':   _create_asm(names, root->left, file, IF_C, WHILE_C);
                        _create_asm(names, root->right, file, IF_C, WHILE_C);
                        fprintf(file, "div\n");
                        break;

            case IF:    _create_asm(names, root->left, file, IF_C++, WHILE_C);
                        fprintf(file, "push 0\n");
                        fprintf(file, "je COND%d:\n", IF_C);
                        fprintf(file, "COND%d:\n", IF_C);
                        _create_asm(names, root->right, file, IF_C+1, WHILE_C);
                        break;

            case WHILE: fprintf(file, "WHILE%d:\n", WHILE_C);
                        _create_asm(names, root->right, file, IF_C, WHILE_C);
                        _create_asm(names, root->left, file, IF_C, WHILE_C);
                        fprintf(file, "push 0\n");
                        fprintf(file, "je WHILE%d:\n", WHILE_C++);
                        break;

        }
    }

    if ((int) NodeValue(root) == ';')  {_create_asm(names, root->left, file, IF_C, WHILE_C); _create_asm(names, root->right, file, IF_C, WHILE_C);}
    return 1;
}

int _var_table(Node * root, Name * names)
{
    if (NodeType(root) == VAR)
    {
        int is_new = 1;
        for (int i = 0; names[i].name; i++)
        {
            if (!strcmp(names[i].name, NodeName(root)))
            {
                is_new = 0;
                break;
            }
        }
        if (is_new)
        {
            names[ADR_C].name = NodeName(root);
            names[ADR_C].address = ADR_C;
            ADR_C++;
        }
    }
    if (root->left) _var_table(root->left, names);
    if (root->right)_var_table(root->right, names);
}

Name * CreateVarTable(Node * root)
{
    Name * names = calloc(1024, sizeof(Name));
    _var_table(root, names);
    return names;
}

int CreateAsm(Tree * tree, const char * filename)
{
    FILE * fp = fopen(filename, "wb");
    if (ferror(fp)) return -1;

    Name * names = CreateVarTable(tree->root);
    if (!names) return NULL;

    _create_asm(names, tree->root, fp, IF_C, WHILE_C);
    fprintf(fp, "hlt\n");

    free(names);
    fclose(fp);

}




