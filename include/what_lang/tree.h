#ifndef TREE_H
#define TREE_H

enum types
{
    ERROR           = -1,
    OPER            =  0,
    VAR             =  1,
    NUM             =  2,
    FUNC_EXT        =  3,
    SEP_SYMB        =  4,
    FUNC_INTER_DEF  =  5,
    FUNC_INTER_CALL =  6

};

typedef double field_t;

typedef struct _field
{
    enum types type;
    field_t value;
    char name[1024];
    int ip;

} Field;

typedef struct _node
{
    void * value;
    struct _node * left;
    struct _node * right;

} Node;


typedef void *  (*TreeInit)     (const void*);
typedef int     (*TreeCmp)      (const void*, const void*);
typedef void    (*TreeFree)     (void*);
// typedef int     (*TreeCb)       (Tree * t, int level, const void*);

typedef struct _tree
{
    Node * root;
    TreeInit init;
    TreeCmp cmp;
    TreeFree free;

} Tree;

static int tree_create_node (Tree * t, Node ** node, const void * pair);
int _tree_parse             (Tree* tree, Node ** node, const char ** string);
Tree * _tree_dump_func      (Tree * tree, Node ** node, FILE * Out);
Node * _insert_tree         (Tree * t, Node ** root, const void * pair);
void _destroy_tree          (Tree * t, Node * n);

Tree * CreateTree           (TreeInit init, TreeCmp cmp, TreeFree free);
int CreateNode              (Tree * t, const void * pair);
int InsertTree              (Tree * t, const void * pair);
int TreeParse               (Tree * tree, const char * filename);
Tree * TreeDump             (Tree * tree, const char * FileName);
void DestroyTree            (Tree * t);

field_t NodeValue(Node * node);
char * NodeName(Node * node);
int NodeIP(Node * node);
enum types NodeType(Node * node);
unsigned int NodeColor(Node * node);

Field * _create_field(field_t val, enum types type);
Node * _create_node(Field * val, Node * left, Node * right);
Field * _copy_field(Field * field);
Node * _copy_node(Node * node);
Node * _copy_branch(Node * node);
Node * _constant_convolution(Node ** node);


#endif
