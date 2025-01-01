#ifndef DIFF_H
#define DIFF_H

typedef struct _tree Tree;

enum types
{
    ERROR = -1,
    OPER = 0,
    VAR = 1,
    NUM = 2,
    FUNC = 3,
    SEP_SYMB = 4
};

typedef double field_t;


typedef struct _field
{
    enum types type;
    field_t value;
    char name[1024];

} Field;

typedef void *  (*TreeInit)    (const void*);
typedef int     (*TreeCmp)      (const void*, const void*);
typedef void    (*TreeFree)     (void*);
typedef int     (*TreeCb)       (Tree * t, int level, const void*);

static int DEF_SIZE = 1024;

enum colors
{
    OPER_COLOR  = 0XEFF94F,
    NUM_COLOR   = 0X5656EC,
    VAR_COLOR   = 0X70Df70,
    FUNC_COLOR  = 0X86E3E3,
    SEP_COLOR   = 0X86B3B3
};

enum operations
{
    ADD         = '+',
    SUB         = '-',
    MUL         = '*',
    DIV         = '/',
    POW         = '^',
    MORE        = '>',
    LESS        = '<',
    ASSIGNMENT  = '=',
    MORE_E      = 70,
    LESS_E      = 71,
    EQUAL       = 72,
    N_EQUAL     = 73,
    DEF         = 74,
};

enum functions
{
    SIN     = 100,
    COS     = 101,
    SQRT    = 102,
    TG      = 103,
    CTG     = 104,
    SH      = 105,
    CH      = 106,
    TH      = 107,
    CTH     = 108,
    LN      = 109,
    LOG     = 110,
    IF      = 111,
    WHILE   = 112,
    PRINT   = 113,
    INPUT   = 114
};

enum errors
{
    SUCCESS,
    ALLOCATE_MEMORY_ERROR,
    MEMCPY_ERROR,
    FOPEN_ERROR,
    FCLOSE_ERROR
};

Tree * CreateTree(TreeInit init, TreeCmp cmp, TreeFree free);

int CreateNode(Tree * t, const void * pair);

int InsertTree(Tree * t, const void * pair);

int TreeParse(Tree * tree, const char * filename);

Tree * TreeDump(Tree * tree, const char * FileName);

int CreateAsm(Tree * tree, const char * filename);

void DestroyTree(Tree * t);


#endif
