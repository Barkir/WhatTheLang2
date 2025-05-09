/*
 * MAJOR        := (OP';')+
 * OPERATOR     := IF';' | A';'
 * FUNCTION     := def ID(ID+) '{' OPERATOR '}';
 * IF           := '('E')' A';' | FUNC';'
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

#include "what_lang/buff.h"
#include "what_lang/parser.h"
#include "what_lang/nametable.h"

#define DEBUG

#define SYNTAX_ERROR(exp, real)                     \
    {                                               \
        SyntaxError(exp, real, __func__, __LINE__); \
    }                                               \

#define NODE_VALUE(node)                            \
{                                                   \
    fprintf(stderr, "%s %d", __func__, __LINE__);   \
    NodeValue(node);                                \
}                                                   \

#ifdef DEBUG
#define PARSER_LOG(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }
#else
#define PARSER_LOG(...)
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
    {.type = FUNC,      .value = PRINT},
    {.type = FUNC,      .value = SQRT},
    {.type = FUNC,      .value = INPUT},

    {.type = OPER,      .value = IF},
    {.type = OPER,      .value = WHILE},
    {.type = OPER,      .value = MORE},
    {.type = OPER,      .value = LESS},
    {.type = OPER,      .value = MORE_E},
    {.type = OPER,      .value = LESS_E},
    {.type = OPER,      .value = EQUAL},
    {.type = OPER,      .value = N_EQUAL},
    {.type = OPER,      .value = DEF},
};


static int tree_create_node(Tree * t, Node ** node, const void * pair);
int _tree_parse(Tree* tree, Node ** node, const char ** string);
Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out);
Node * _insert_tree(Tree * t, Node ** root, const void * pair);
void _destroy_tree(Tree * t, Node * n);

const char * _enum_to_name(int name);

void SyntaxError(char exp, char real, const char * func, int line);
Node ** StringTokenize(const char * string, int * p);

Node * GetMajor(Node ** nodes, int * p);
Node * GetOperator(Node ** nodes, int * p);
Node * GetAssignment(Node ** nodes, int * p);
Node * GetExpression(Node ** nodes, int * p);
Node * GetTerm(Node ** nodes, int * p);
Node * GetPow(Node ** nodes, int * p);
Node * GetCompare(Node ** nodes, int * p);
Node * GetBracket(Node ** nodes, int * p);
Node * GetFunc(Node ** nodes, int * p);
Node * GetFuncName(Node ** nodes, int * p);
Node * GetNumber(Node ** nodes, int * p);
Node * GetID(Node ** nodes, int * p);
Node * GetIf(Node ** nodes, int * p);
Node * GetWhile(Node ** nodes, int * p);
Node * GetGroup(Node ** nodes, int * p);
Node * GetParam(Node ** nodes, int * p);
Node * GetCall(Node ** nodes, int * p);

Node * _copy_branch(Node * node);
Node * _copy_node(Node * node);
Field * _copy_field(Field * field);
Node * _create_node(Field * val, Node * left, Node * right);
Field * _create_field(field_t val, enum types type);

unsigned int NodeColor(Node * node);
field_t NodeValue(Node * node);
enum types NodeType(Node * node);
char * NodeName(Node * node);

Name * CreateVarTable(Node * root);
Name  * CreateFuncTable(Node * root);
int _var_table(Node * root, Name * names, const char * func_name);
int _func_table(Node * root, Name * names);
int GetVarAdr(Node * root, Name * names);
const char * GetVarName(Node * root);

Node * _get_token(const char * string, int * p);
Node * _sep_token(const char * string, int * p);
Node * _number_token(const char * string, int * p);
Node * _name_token(const char * string, int * p);
Node * _oper_token(const char * string, int * p);
Node * _find_name(char * result);
int _is_oper(const char * string, int * p);

int _create_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count);

static int IF_COUNT = 0;
static int WHILE_COUNT = 0;
static int ADR_COUNT = 0;

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
        return tree_create_node(t, &(*node)->right, pair);
    else
        return tree_create_node(t, &(*node)->left, pair);

    (*node)->left = (*node)->right = NULL;

    return 0;
}

int CreateNode(Tree * t, const void * pair)
{
    return tree_create_node(t, &t->root, pair);
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
    return (_insert_tree(t, &t->root, pair) != NULL);
}

Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out)
{

    if (!tree)
        return NULL;

    if (!*node) return NULL;

    unsigned int color = NodeColor(*node);
    field_t field = NodeValue(*node);

    switch ((int) NodeType(*node))
    {
        case OPER:
                    // PARSER_LOG("OPER");
                    // PARSER_LOG("node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    // *node, _enum_to_name((int) field), *node, color);

                    fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, _enum_to_name((int) field), *node, color);
                    break;

        case VAR:   PARSER_LOG("VAR name = %s", NodeName(*node));
                    fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, NodeName(*node), *node, color);
                    break;

        case NUM:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, NodeValue(*node), *node, color);
                    break;

        case FUNC:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, _enum_to_name((int) field), *node, color);
                    break;

        case SEP_SYMB:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                        *node, (int) field, color);
                        break;

        case FUNC_NAME: fprintf(Out, "node%p [shape = Mrecord; label = \"{%s}\"; style = filled; fillcolor = \"#%06X\"];\n",
                        *node, NodeName(*node), color);
                        break;

        default:    fprintf(Out, "node%p [shape = Mrecord; label = \"{}\"; style = filled; fillcolor = \"#%06X\"];\n",
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
    if (!node) return -1;
    // PARSER_LOG("Getting node value %p %lg(%c)", node, ((Field*)(node->value))->value, (int)((Field*)(node->value))->value);
    // return ((Field*)(node->value))->value;
    return ((Field*)((char*)node + sizeof(Node)))->value;
}

char * NodeName(Node * node)
{
    if (!node) return NULL;
    // PARSER_LOG("Getting node name %p %s", node, ((Field*)(node->value))->name);
    // return ((Field*)(node->value))->name;
    return ((Field*)((char*)node + sizeof(Node)))->name;
}

enum types NodeType(Node * node)
{
    if (!node) return ERROR;
    // return ((Field*)(node->value))->type;
    return ((Field*)((char*)node + sizeof(Node)))->type;
}

unsigned int NodeColor(Node * node)
{
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

        case FUNC:
            color = FUNC_COLOR;
            break;

        case SEP_SYMB:
            color = SEP_COLOR;
            break;

        case FUNC_NAME:
            color = FUNC_NAME_COLOR;
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

#define SKIPSPACE while(isspace(string[*p]) || (string[*p]) == ',') (*p)++

void SyntaxError(char exp, char real, const char * func, int line)
{
    fprintf(stderr, ">>> SyntaxError %s %d: <expected %c> <got %c (%lg)>", func, line, exp, (int) real, (field_t) real);
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
    if (strcmp(name, "print") == 0)     return PRINT;
    if (strcmp(name, "sqrt") == 0)      return SQRT;
    if (strcmp(name, "def") == 0)       return DEF;
    if (strcmp(name, "input") == 0)     return INPUT;


    if (strcmp(name, "+") == 0)         return '+';
    if (strcmp(name, "-") == 0)         return '-';
    if (strcmp(name, "*") == 0)         return '*';
    if (strcmp(name, "/") == 0)         return '/';
    if (strcmp(name, "^") == 0)         return '^';
    if (strcmp(name, "=") == 0)         return '=';
    if (strcmp(name, "(") == 0)         return '(';
    if (strcmp(name, ")") == 0)         return ')';
    if (strcmp(name, "{") == 0)         return '{';
    if (strcmp(name, "}") == 0)         return '}';

    if (strcmp(name, ">") == 0)         return MORE;
    if (strcmp(name, "<") == 0)         return LESS;
    if (strcmp(name, ">=") == 0)        return MORE_E;
    if (strcmp(name, ">=") == 0)        return MORE_E;
    if (strcmp(name, "==") == 0)        return EQUAL;
    if (strcmp(name, "!=") == 0)        return N_EQUAL;


    PARSER_LOG("Can't find name %s", name);
    return -1;
}

const char * _enum_to_name(int name)
{
    switch(name)
    {
        case MORE:       return "more";
        case LESS:       return "less";
        case MORE_E:     return "more_e";
        case LESS_E:     return "less_e";
        case EQUAL:      return "equal";
        case N_EQUAL:    return "n_equal";
        case DEF:        return "def";

        case '+':        return "+";
        case '=':        return "=";
        case '/':        return "//";
        case '*':        return "*";
        case '^':        return "^";
        case '-':        return "-";
        case '(':        return "(";
        case ')':        return ")";
        case '{':        return "{";
        case '}':        return "}";

        case SIN:        return "sin";
        case COS:        return "cos";
        case SQRT:       return "sqrt";
        case SH:         return "sh";
        case CH:         return "ch";
        case TG:         return "tg";
        case LOG:        return "log";
        case CTG:        return "ctg";
        case LN:         return "ln";
        case TH:         return "th";
        case CTH:        return "cth";
        case IF:         return "if";
        case WHILE:      return "while";
        case PRINT:      return "print";
        case INPUT:      return "input";

        default: return "notfound";
    }
}

Node * _find_name(char * result)
{
    PARSER_LOG("Need to find name %s... ", result);
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
    SKIPSPACE;

    int start_p = *p;

    while(string[*p] == '=' || string[*p] == '>' || string[*p] == '<' || string[*p] == '!' || string[*p] == '+' \
            || string[*p] == '/' || string[*p] == '*' || string[*p] == '+' || string[*p] == '-'
            || string[*p] == '(' || string[*p] == ')' || string[*p] == '{' || string[*p] == '}')
    {
        if (string[*p] == '(' || string[*p] == ')' || string[*p] == '{' || string[*p] == '}')
        {
            (*p)++;
            break;
        }
        (*p)++;
    }

    char * oper = (char*) calloc((*p) - start_p + 1, 1);
    memcpy(oper, &string[start_p], (*p) - start_p);
    PARSER_LOG("oper = %s", oper);

    Field * field = _create_field((field_t) _name_to_enum(oper), OPER);
    if  (!field) return NULL;
    Node * result = _create_node(field, NULL, NULL);

    PARSER_LOG("result = %p -> [%d, %c]", result, (int) NodeValue(result), (int) NodeValue(result));

    if (!result) return NULL;
    return result;
}

Node * _name_token(const char * string, int * p)
{
    SKIPSPACE;
    int start_p = *p;

    while(isalpha(string[*p])) (*p)++;

    const char * result = (const char*) calloc((*p) - start_p + 1, 1);
    memcpy(result, &string[start_p], (*p) - start_p);

    Node * name = _find_name(result);
    free(result);
    if (!name) return NULL;

    PARSER_LOG("name = %p -> [%s]", name, NodeName(name));

    return name;
}

Node * _number_token(const char * string, int * p)
{
    SKIPSPACE;
    char * end = NULL;
    field_t number = strtof(&(string[*p]), &end);
    if (!end) return NULL;
    (*p) += (int)(end - &string[*p]);
    Field * field = _create_field(number, NUM);
    if (!field) return NULL;
    Node * num = _create_node(field, NULL, NULL);
    if (!num) return NULL;

    PARSER_LOG("number = %p -> [%lg]", num, NodeValue(num));

    return num;
}

Node * _sep_token(const char * string, int * p)
{
    SKIPSPACE;
    Field * field = _create_field((field_t) ';', SEP_SYMB);
    if (!field) return NULL;
    Node * num = _create_node(field, NULL, NULL);
    if (!num) return NULL;
    (*p)++;
    return num;
}

Node * _get_token(const char * string, int * p)
{
    SKIPSPACE;
    if (_is_oper(string, p)) {PARSER_LOG("operator %c! ", string[*p]); return _oper_token(string, p);}

    if (string[*p] == ';') {PARSER_LOG("separator of line!"); return _sep_token(string, p);}

    if (isalpha(string[*p])) {PARSER_LOG("name %c! ", string[*p]); return _name_token(string, p);}

    if (isdigit(string[*p])) {PARSER_LOG("number %c! ", string[*p]); return _number_token(string, p);}

    return NULL;

}

Node ** StringTokenize(const char * string, int * p)
{
    SKIPSPACE;
    int size = 0;
    size_t arr_size = (size_t) DEF_SIZE;
    Node ** nodes = (Node**) calloc(arr_size, sizeof(Node*));
    while (string[*p] != 0)
    {
        *(nodes + size) = _get_token(string, p);
        PARSER_LOG("got node %d %p with value %lg!\n", size+1, *(nodes + size), NodeValue(*(nodes + size)));
        size++;
    }
    return nodes;
}

Node * GetMajor(Node ** nodes, int * p)
{
    PARSER_LOG("Got node %p", nodes[*p]);
    Node * result = NULL;
    Node * operation = NULL;
    Field * oper = NULL;
    while (operation = GetOperator(nodes, p))
    {
        oper = _create_field(';', SEP_SYMB);
        result = _create_node(oper, result, operation);
    }
    PARSER_LOG("Got result!");
    if (!nodes[*p]) return result;
    PARSER_LOG("PARSING ENDED");
    (*p)++;
    return result;
}

Node * GetOperator(Node ** nodes, int * p)
{
    int old_p = (*p);
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Getting O... Got node %p with val %lg, name %s, p = %d", nodes[*p], NodeValue(nodes[*p]), NodeName(nodes[*p]), *p);
    Node * val1 = NULL;
    if ((val1 = GetAssignment(nodes, p)))
    {
        PARSER_LOG("Got Assignment...");
        if ((char) NodeValue(nodes[*p]) == ';')
        {
            PARSER_LOG("Got ';'");
            (*p)++;
            return val1;
        }
        if ((int) NodeValue(nodes[*p]) == -1) return val1;
        SYNTAX_ERROR(';', (char) NodeValue(nodes[*p]));
    }

    if ((val1 = GetIf(nodes, p)))
    {
        PARSER_LOG("Got If...");
        (*p)--;
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER_LOG("Got ';'");
            (*p)++;
            return val1;
        }

        if ((int) NodeValue(nodes[*p]) == '}')
        {
            PARSER_LOG("Got '}'");
            (*p)++;
            return val1;
        }
        if ((int) NodeValue(nodes[*p]) == -1) return val1;
        SYNTAX_ERROR(';', (char) NodeValue(nodes[*p]));
    }
    if ((val1 = GetWhile(nodes, p)))
    {
        PARSER_LOG("Got While...");
        (*p)--;
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER_LOG("Got ';'");
            (*p)++;
            return val1;
        }

        if ((int) NodeValue(nodes[*p]) == '}')
        {
            PARSER_LOG("Got '}'");
            (*p)++;
            return val1;
        }

        if ((int) NodeValue(nodes[*p]) == -1) return val1;
        SYNTAX_ERROR(';', (char) NodeValue(nodes[*p]));
    }

    if (val1 = GetFunc(nodes, p))
    {
        PARSER_LOG("Got func...");
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER_LOG("Got ;");
            (*p)++;
            return val1;
        }
        if ((int) NodeValue(nodes[*p]) == -1) return val1;
        SYNTAX_ERROR(';', (char) NodeValue(nodes[*p]));
    }

    if (val1 = GetFuncName(nodes, p))
    {
        PARSER_LOG("Got FuncName...");
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER_LOG("Got ;");
            (*p)++;
            return val1;
        }
        if ((int) NodeValue(nodes[*p]) == -1) return val1;
        SYNTAX_ERROR(';', (char) NodeValue(nodes[*p]));
    }

    if (val1 = GetCall(nodes, p))
    {
        PARSER_LOG("Got Call...");
        (*p)--;
        if ((int) NodeValue(nodes[*p]) == ';')
        {
            PARSER_LOG("Got ';'");
            (*p)++;
            return val1;
        }

        if ((int) NodeValue(nodes[*p]) == '}')
        {
            PARSER_LOG("Got '}'");
            (*p)++;
            return val1;
        }
        if ((int) NodeValue(nodes[*p]) == -1) return val1;
        SYNTAX_ERROR(';', (char) NodeValue(nodes[*p]));
    }
    (*p) = old_p;
    return NULL;
}

Node * GetCall(Node ** nodes, int * p)
{
    if ((int) NodeValue(nodes[*p]) != DEF) return NULL;
    Field * field = _copy_field(((Field*)((char*)(nodes[*p]) + sizeof(Node))));
    PARSER_LOG("Getting CALL... p = %d", *p);
    (*p)++;
    Node * name = GetID(nodes, p);
    ((Field*)((char*)name + sizeof(Node)))->type = FUNC_NAME;

    Node * group = GetGroup(nodes, p);
    return _create_node(field, name, group);
}

Node * GetGroup(Node ** nodes, int * p)
{
    if ((int) NodeValue(nodes[*p]) != '{') return NULL;
    (*p)++;
    PARSER_LOG("Getting Group!");
    Node * result = NULL;
    Node * val = NULL;
    Field * sep = NULL;

    while (val = GetOperator(nodes, p))
    {
        sep = _create_field((field_t) ';', SEP_SYMB);
        result = _create_node(sep, result, val);
    }
    if ((int) NodeValue(nodes[*p]) != '}' && (int) NodeValue(nodes[*p]) != -1) SYNTAX_ERROR('}', (int) NodeValue(nodes[*p]));
    (*p)++;
    return result;
}

Node * GetIf(Node ** nodes, int * p)
{
    if ((int) NodeValue(nodes[*p]) != IF) return NULL;
    PARSER_LOG("Getting IF, p = %d", *p);
    int old_p = (*p);
    Node * E = NULL;
    Node * A = NULL;

    Field * oper = _copy_field((Field*)((char*)nodes[*p] + sizeof(Node)));
    (*p)++;

    if ((E = GetBracket(nodes, p)) && (A = GetOperator(nodes, p)))
        return _create_node(oper, E, A);

    (*p) = old_p;
    (*p)++;
    if ((E = GetBracket(nodes, p)) && (A = GetGroup(nodes, p)))
        return _create_node(oper, E, A);

    (*p) = old_p;
    return NULL;
}

Node * GetWhile(Node ** nodes, int * p)
{
    if ((int) NodeValue(nodes[*p]) != WHILE) return NULL;
    PARSER_LOG("Getting WHILE");
    int old_p = (*p);

    Node * E = NULL;
    Node * A = NULL;

    Field * oper = _copy_field((Field*)((char*)nodes[*p] + sizeof(Node)));
    (*p)++;

    if ((E = GetBracket(nodes, p)) && (A = GetOperator(nodes, p)))
        return _create_node(oper, E, A);

    (*p) = old_p;
    (*p)++;

    if ((E = GetBracket(nodes, p)) && (A = GetGroup(nodes, p)))
        return _create_node(oper, E, A);

    (*p) = old_p;
    return NULL;
}

Node * GetAssignment(Node ** nodes, int * p)
{
    if (NodeType(nodes[*p]) != VAR || NodeType(nodes[(*p) + 1]) != OPER || (int) NodeValue(nodes[(*p) + 1]) != '=') return NULL;
    int old_p = (*p);
    PARSER_LOG("Getting A... Got node %p %d, p = %d", nodes[*p], NodeType(nodes[*p]), *p);
    if (NodeType(nodes[*p]) != VAR) return NULL;
    Node * val1 = GetID(nodes, p);
    if (!val1) return  NULL;
    PARSER_LOG("Got ID node %p with name %s", val1, NodeName(val1));
    if ((int)NodeValue(nodes[*p]) == '=')
    {
        Field * operation = _create_field((field_t) '=', OPER);
        PARSER_LOG("Got '=', p = %d", *p);
        (*p)++;
        Node * val2 = GetExpression(nodes, p);
        if (!val2)
        {
            (*p) = old_p;
            return NULL;
        }
        val1 = _create_node(operation, val1, val2);
        PARSER_LOG("val1 = %p, value = %lg, left = %p(%lg), right=%p(%lg)", val1, NodeValue(val1), val1->left, NodeValue(val1->left), val1->right, NodeValue(val1->right));
        return val1;
    }
    (*p) = old_p;
    return NULL;

}

Node * GetExpression(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Getting E... Got node %p, p = %d", nodes[*p], *p);
    Node * val1 = GetTerm(nodes, p);
    if (!nodes[*p]) return val1;

    while ((int)NodeValue(nodes[*p]) == '+' || (int)NodeValue(nodes[*p]) == '-')
    {
        PARSER_LOG("Got node %p", nodes[*p]);
        int op = (int) NodeValue(nodes[*p]);

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        (*p)++;
        if (!nodes[*p]) return NULL;

        Node * val2 = GetTerm(nodes, p);
        PARSER_LOG("Got val2");

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER_LOG("GetExpression Finished");
    return val1;
}

Node * GetTerm(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Getting T... Got node %p", nodes[*p]);
    PARSER_LOG("Getting pow in T val1..."); Node * val1 = GetPow(nodes, p);
    if (!nodes[*p]) {PARSER_LOG("GetTerm Finished!"); return val1;}

    while ((int)NodeValue(nodes[*p]) == '*' || (int)NodeValue(nodes[*p]) == '/')
    {
        int op = (int)NodeValue(nodes[*p]);

        Field * operation = NULL;

        if (op == '*') operation = _create_field((field_t) op, OPER);
        else if (op == '/') operation = _create_field((field_t) op, OPER);

        if (!operation) return NULL;


        (*p)++;
        if (!nodes[*p]) return val1;

        PARSER_LOG("Getting pow in T val2..."); Node * val2 = GetPow(nodes, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER_LOG("GetTerm Finished");
    return val1;
}

Node * GetPow(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Getting pow... Got node %p", nodes[*p]);
    Node * val1 = GetCompare(nodes, p);
    if (!nodes[*p]) {PARSER_LOG("GetPow Finished"); return val1;}
    while ((int)NodeValue(nodes[(*p)]) == '^')
    {
        PARSER_LOG("Got '^'");
        int op = (int) NodeValue(nodes[(*p)]);

        Field * operation = NULL;

        (*p)++;
        if (!nodes[*p]) return val1;

        Node * val2 = GetCompare(nodes, p);
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER_LOG("GetPow Finished");
    return val1;
}

Node * GetCompare(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Getting pow... Got node %p", nodes[*p]);
    Node * val1 = GetBracket(nodes, p);
    if (!nodes[*p]) {PARSER_LOG("GetPow Finished"); return val1;}
    while (((int)NodeValue(nodes[(*p)]) == MORE || (int)NodeValue(nodes[*p]) == LESS
            || (int)NodeValue(nodes[*p]) == MORE_E || (int)NodeValue(nodes[*p]) == LESS_E
            || (int)NodeValue(nodes[*p]) == EQUAL  || (int)NodeValue(nodes[*p]) == N_EQUAL) && NodeType(nodes[*p]) == OPER)
    {
        PARSER_LOG("Got compare");
        int op = (int) NodeValue(nodes[(*p)]);

        Field * operation = NULL;

        (*p)++;
        if (!nodes[*p]) return val1;

        Node * val2 = GetBracket(nodes, p);
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER_LOG("GetPow Finished");
    return val1;
}


Node * GetBracket(Node ** nodes, int * p)
{
    int old_p = (*p);
    if (!nodes[*p]) return NULL;
    Node * val = NULL;
    PARSER_LOG("node = %p. Getting P...", nodes[*p]);
    if ((int) NodeValue(nodes[*p]) == '(' && NodeType(nodes[*p]) == OPER)
    {
        PARSER_LOG("Got '(', p = %d", *p);
        (*p)++;
        if ((val = GetExpression(nodes, p)) && (NodeValue(nodes[(*p)]) == ')') && (NodeType(nodes[*p]) == OPER))
        {
            (*p)++;
            return val;
        }

        PARSER_LOG("Got node %p", nodes[*p]);
        if (!nodes[*p]) return val;
        if ((char) NodeValue(nodes[(*p)]) != ')' || NodeType(nodes[*p]) != OPER) SYNTAX_ERROR(')', (char) NodeValue(nodes[(*p)]));
        PARSER_LOG("Got ')', p = %d", *p);
        (*p)++;
        return val;
    }

    if (NodeType(nodes[*p]) == FUNC_NAME) return GetFuncName(nodes, p);
    else if (NodeType(nodes[*p]) == FUNC) return GetFunc(nodes, p);
    else if (NodeType(nodes[*p]) == VAR)  return GetID(nodes, p);
    else return GetNumber(nodes, p);
}

Node * GetFuncName(Node ** nodes, int * p)
{
    if (NodeType(nodes[*p]) != FUNC_NAME && NodeType(nodes[*p]) != VAR) return NULL;
    PARSER_LOG("Getting FUNC_NAME");
    Node * result = _copy_node(nodes[*p]);
    if (!result) return NULL;
    (*p)++;

    Node * val = GetParam(nodes, p);
    if (!val) return NULL;
    result->left = val;
    (*p)++;
    return result;

}

Node * GetFunc(Node ** nodes, int * p)
{
    if ((int) NodeType(nodes[*p]) != FUNC) return NULL;
    PARSER_LOG("Getting FUNC");
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Got node %p %lg", nodes[*p], NodeValue(nodes[*p]));
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    PARSER_LOG("Got node %p %lg ('%c')", nodes[*p], NodeValue(nodes[*p]), (int) NodeValue(nodes[*p]));


    Node * val = GetExpression(nodes, p);
    if (!val) return NULL;
    result->left = val;
    return result;
}

Node * GetNumber(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

Node * GetParam(Node ** nodes, int * p)
{
    PARSER_LOG("GETTING PARAM");
    Node * result = nodes[(*p) - 1];
    Node *  dummy = nodes[(*p) - 1];
    PARSER_LOG("FUNCTION = %s", NodeName(result));
    (*p)++;
    while (NodeType(nodes[*p]) == VAR || NodeType(nodes[*p]) == NUM)
    {
        Node * left = (Node*)((char*)dummy + sizeof(Field));
        dummy->left = _copy_node(nodes[*p]);
        dummy = dummy->left;

        (*p)++;
        PARSER_LOG("p = %d", *p);
    }
    PARSER_LOG("FINISHED GETTING PARAM");

    return result->left;
}

Node * GetID(Node ** nodes, int * p)
{
    if (NodeType(nodes[*p]) != VAR) return NULL;
    PARSER_LOG("Got node %p with name %s, p = %d", nodes[*p], NodeName(nodes[*p]), *p);
    Node * result = _copy_node(nodes[*p]);
    if (!result) return NULL;
    (*p)++;
    if (NodeType(nodes[*p]) == OPER && (int) NodeValue(nodes[*p]) == '(')
    {
        PARSER_LOG("Got FUNCTION WITH PARAM");
        result->left = GetParam(nodes, p);
        if (NodeType(nodes[*p]) != OPER || (int) NodeValue(nodes[*p]) != ')') SYNTAX_ERROR(')', NodeValue(nodes[*p]));
        (*p)++;
    }

    PARSER_LOG("result = %p [%s]", result, NodeName(result));

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
        if (!strcmp(NodeName(root), names[i].name) && names[i].type == VAR) return names[i].address;
    }

    return -1;
}

Name * GetFuncAdr(Node * root, Name * names)
{
    for(int i = 0; i < 1024; i++)
    {
        if (!(names[i].name)) return NULL;
        if (!strcmp(NodeName(root), names[i].name) && names[i].type == FUNC_NAME) return &(names[i]);
    }

    return NULL;
}

const char * Adr2Reg(int adr)
{
    switch(adr)
    {
        case 0:     return "rbx";
        case 1:     return "rcx";
        case 2:     return "rdx";
        case 3:     return "rsx";
        case 4:     return "rdi";
        case 5:     return "r11";
        default:    return "r12";
    }
}

int _create_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    if (NodeType(root) == NUM)
    {
        fprintf(file, "push %lg\n", NodeValue(root));
    }
    if (NodeType(root) == VAR)
    {
        int adr = GetVarAdr(root, names);
        if (adr >= 0) fprintf(file, "push %s\n", Adr2Reg(adr));
        else
        {
            Name * func = GetFuncAdr(root, names);
            Node * dummy = root->left;

            if (func->param != 0)
            {
                for (int i = func->address; dummy; i++)
                {
                    fprintf(file, "push %lg\n", NodeValue(dummy));
                    fprintf(file, "pop %s\n", Adr2Reg(names[i].address));
                    dummy = dummy->left;
                }
            }
            fprintf(file, "call %s\n", NodeName(root));
        }
    }

    if (NodeType(root) == OPER)
    {
        int local_if = 0;
        int local_while = 0;

        switch ((int) NodeValue(root))
        {
            case '=':   _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "pop %s             ; '=' operation\n", Adr2Reg(GetVarAdr(root->left, names)));
                        break;

            case '+':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'+' operation\n\n");

                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop r15\n");

                        fprintf(file, "add r14, r15\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case '-':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'-' operation\n\n");

                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop r15\n");

                        fprintf(file, "sub r15, r14\n");
                        fprintf(file, "push r15\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case '*':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'*' operation\n\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop rax\n");

                        fprintf(file, "mul r14\n");
                        fprintf(file, "push rax\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case '/':   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";'/' operation\n\n");

                        fprintf(file, "pop r14\n");
                        fprintf(file, "pop rax\n");


                        fprintf(file, "div r14\n");
                        fprintf(file, "push rax\n");
                        fprintf(file, ";---------------------------\n\n");
                        break;

            case MORE:      _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'>' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "ja SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "ja SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");

                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");
                            break;

            case LESS:      _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, "'<' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jb SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jb SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case MORE_E:    _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'>=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jae SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jae SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case LESS_E:    _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'<=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jbe SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jbe SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case EQUAL:     _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'==' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "je SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "je SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;

            case N_EQUAL:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                            _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);

                            fprintf(file, ";---------------------------\n");
                            fprintf(file, ";'!=' comparsion\n\n");

                            fprintf(file, "pop r15\n");
                            fprintf(file, "pop r14\n");
                            fprintf(file, "push r14\n");
                            fprintf(file, "push r15\n");

                            fprintf(file, "cmp r14, r15\n");

                            if (if_cond)           fprintf(file, "jne SUB_COND%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jne SUB_COND%d\n", while_count);

                            fprintf(file, "push 0\n");
                            if (if_cond)           fprintf(file, "jmp IF_END%d\n", if_count);
                            else if (while_cond)   fprintf(file, "jmp WHILE_FALSE%d\n", while_count);

                            if          (if_cond) fprintf(file, "SUB_COND%d:\n", if_count);
                            else if     (while_cond) fprintf(file, "SUB_COND%d:\n", while_count);

                            fprintf(file, "push 1\n");
                            if (if_cond)           fprintf(file, "jmp IF%d\n", if_count++);
                            else if (while_cond)   fprintf(file, "jmp WHILE_TRUE%d\n", while_count++);

                            fprintf(file, ";---------------------------\n\n");

                            break;


            case IF:
                        local_if = IF_COUNT;
                        if_count = IF_COUNT;
                        IF_COUNT++;

                        _create_asm(names, root->left, file, 1, 0, if_count, while_count);

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";if condition\n\n");

                        fprintf(file, "IF%d:\n", if_count);
                        fprintf(file, "push 0\n");


                        fprintf(file, "pop r15\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, "push r15\n");

                        fprintf(file, "cmp r14, r15\n");

                        fprintf(file, "jne COND%d\n", if_count);

                        fprintf(file, "jmp IF_END%d\n", if_count);

                        fprintf(file, "COND%d:\n", if_count);
                        if_count++;
                        _create_asm(names, root->right, file, 1, 0, if_count, while_count);

                        fprintf(file, "IF_END%d:\n", local_if);

                        fprintf(file, ";---------------------------\n\n");
                        break;


            case WHILE:
                        local_while = WHILE_COUNT;
                        while_count = WHILE_COUNT;
                        WHILE_COUNT++;

                        fprintf(file, ";---------------------------\n");
                        fprintf(file, ";while condition\n\n");
                        fprintf(file, "WHILE%d:\n", while_count);
                        _create_asm(names, root->left, file, 0, 1, if_count, while_count);

                        fprintf(file, "WHILE_FALSE%d:\n", while_count);
                        fprintf(file, "push 0\n");

                        fprintf(file, "pop r15\n");
                        fprintf(file, "pop r14\n");
                        fprintf(file, "push r14\n");
                        fprintf(file, "push r15\n");

                        fprintf(file, "cmp r14, r15\n");

                        fprintf(file, "je WHILE_END%d\n", while_count);

                        fprintf(file, "WHILE_TRUE%d:\n", while_count);
                        _create_asm(names, root->right, file, 0, 1, if_count, while_count);

                        fprintf(file, "jmp WHILE%d\n", local_while);
                        fprintf(file, "WHILE_END%d:\n", local_while);

                        fprintf(file, ";---------------------------\n\n");
                        break;

            case DEF:   break;


            default: return -1;


        }
    }
    if (NodeType(root) == FUNC)
    {
        switch((int) NodeValue(root))
        {
            case SIN:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sin\n");
                        break;

            case COS:   _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "cos\n");
                        break;

            case SQRT:  _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "sqrt\n");
                        break;

            case PRINT: _create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
                        fprintf(file, "out\n");
                        break;

            case INPUT: fprintf(file, "in\n");
                        break;

            default: return -1;
        }
    }

    if (NodeType(root) == FUNC_NAME) fprintf(file, "call %s\n", NodeName(root));

    if ((int) NodeValue(root) == ';')  {_create_asm(names, root->left, file, if_cond, while_cond, if_count, while_count); _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);}
    return 1;
}

int _def_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count)
{
    PARSER_LOG("DEF_ASM...");
    if (NodeType(root) == OPER && (int) NodeValue(root) == DEF)
    {
        Node * param = root;
        int i = 0;

        fprintf(file, "%s:\n", NodeName(root->left));
        _create_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);
        fprintf(file, "ret\n");
    }
    if (root->left)  _def_asm(names, root->left, file, if_cond, while_cond, if_count, while_count);
    if (root->right) _def_asm(names, root->right, file, if_cond, while_cond, if_count, while_count);
    return 1;
}

int _count_param(Node * root)
{
    if (NodeType(root) == VAR) return 0;
    int i = 0;
    Node * dummy = root;
    while (dummy->left)
    {
        i++;
        dummy = dummy->left;
    }
    return i;
}

int _var_table(Node * root, Name * names, const char * func_name)
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
            names[ADR_COUNT].name = NodeName(root);
            names[ADR_COUNT].address = ADR_COUNT?(names[ADR_COUNT - 1].address_end + 1) : ADR_COUNT;
            names[ADR_COUNT].param = _count_param(root);
            names[ADR_COUNT].address_end = names[ADR_COUNT].address + names[ADR_COUNT].param;
            names[ADR_COUNT].type = VAR;
            names[ADR_COUNT].func_name = func_name;
            ADR_COUNT++;
        }
    }
    if (root->left)
        {
            if (NodeType(root) == FUNC_NAME) func_name = NodeName(root);
            _var_table(root->left, names, func_name);
        }
    if (root->right)
            {
            if (NodeType(root) == OPER && (int) NodeValue(root) == DEF) func_name = NodeName(root);
            _var_table(root->right, names, func_name);
        }
    return 0;
}

int _func_table(Node * root, Name * names)
{
    if (NodeType(root) == OPER && NodeValue(root) == DEF)
    {
        int is_new = 1;
        for (int i = 0; names[i].name; i++)
        {
            if (!strcmp(names[i].name, NodeName(root->left)))
            {
                is_new = 0;
                break;
            }
        }
        if (is_new)
        {
            names[ADR_COUNT].name = NodeName(root->left);
            names[ADR_COUNT].address = ADR_COUNT;
            names[ADR_COUNT].type = FUNC_NAME;
            names[ADR_COUNT].param = _count_param(root->left);
            names[ADR_COUNT].address_end = names[ADR_COUNT].address + names[ADR_COUNT].param;
            ADR_COUNT++;
        }
    }
    if (root->left) _func_table(root->left, names);
    if (root->right)_func_table(root->right, names);
    return 0;
}

Name * CreateVarTable(Node * root)
{
    Name * names = calloc(1024, sizeof(Name));
    _var_table(root, names, "");
    return names;
}

Name * CreateFuncTable(Node * root)
{
    Name * funcs = calloc(1024, sizeof(Name));
    _func_table(root, funcs);
    return funcs;
}

int _find_func_start(Name * names, const char * func_name)
{
    for (int i = 0; names[i].name; i++)
    {
        if (!strcmp(names[i].func_name, func_name)) return names[i].address;
    }
    return -1;

}

int _find_func_end(Name * names, const char * func_name)
{
    int end = -1;
        for (int i = 0; names[i].name; i++)
    {
        if (!strcmp(names[i].func_name, func_name)) end = end > names[i].address ? end : names[i].address;
    }

    return end;

}

int CreateAsm(Tree * tree, const char * filename)
{
    PARSER_LOG("Creating ASM....");
    FILE * fp = fopen(filename, "wb");
    if (ferror(fp)) return -1;

    Name * names = CreateVarTable(tree->root);
    ADR_COUNT = 0;

    Name * func  = CreateFuncTable(tree->root);

    for (int i = 0; names[i].name; i++)
    {
        for(int j = 0; func[j].name; j++)
        {
            if (!strcmp(names[i].name, func[j].name))
            {
                PARSER_LOG("FOUND FUNC %s", func[j].name);
                names[i].name = func[j].name;
                names[i].param = func[j].param;
                names[i].type = func[j].type;
                names[i].address = _find_func_start(names, names[i].name);
                names[i].address_end = _find_func_end(names, names[i].name);
                break;
            }
        }
    }
    if (!names) return -1;

    for (int i = 0; names[i].name; i++)
    {
        // printf("%s(param: %d, type: %d, func_name = %s): starts at %d, ends at %d\n", names[i].name, names[i].param, names[i].type, names[i].func_name, names[i].address, names[i].address_end);
    }

    fprintf(fp, "section .text\n");
    fprintf(fp, "global _start\n");
    fprintf(fp, "_start:\n");
    _create_asm(names, tree->root, fp, 0, 0, 0, 0);

    fprintf(fp, "mov rax, 60\n");
    fprintf(fp, "syscall\n");

    _def_asm(names, tree->root, fp, 0, 0, 0, 0);

    free(names);
    fclose(fp);
    return 0;

}

int _is_oper(const char * string, int * p)
{
        return string[*p] == '(' ||
        string[*p] == ')' ||
        string[*p] == '{' ||
        string[*p] == '}' ||
        string[*p] == '+' ||
        string[*p] == '-' ||
        string[*p] == '/' ||
        string[*p] == '*' ||
        string[*p] == '^' ||
        string[*p] == '>' ||
        string[*p] == '<' ||
        (string[*p] == '!' && string[(*p) + 1] == '=') ||
        (string[*p] == '=' && string[(*p) + 1] != '=') ||
        (string[*p] == '=' && string[(*p) + 1] == '=');
}

