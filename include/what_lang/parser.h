#ifndef DIFF_H
#define DIFF_H

const int VAL2REG_START = 0xb8;

const char PUSHREG_BYTE = 0x50;
const char PUSHIMM32_BYTE = 0x68;
const char POP_BYTE  = 0x59;
const char CALL_DIRECT_BYTE = 0xe8;
const char ADDITIONAL_REG_BYTE = 0x41;


const char OPER_BYTE = 0x48;
const char XTEND_OPER_BYTE = 0x49;


enum Registers
{
    REG_EAX = 0x00,
    REG_ECX = 0x01,
    REG_EDX = 0x02,
    REG_EBX = 0x03,
    REG_ESP = 0x04,
    REG_EBP = 0x05,
    REG_ESI = 0x06,
    REG_EDI = 0x07,
};

enum AdditionalRegisters
{
    REG_R8  = 0x00,
    REG_R9  = 0x01,
    REG_R10 = 0x02,
    REG_R11 = 0x03,
    REG_R12 = 0x04,
    REG_R13 = 0x05,
    REG_R14 = 0x06,
    REG_R15 = 0x07,
};

typedef struct _tree Tree;

enum types
{
    ERROR = -1,
    OPER = 0,
    VAR = 1,
    NUM = 2,
    FUNC = 3,
    SEP_SYMB = 4,
    FUNC_NAME = 5
};

typedef double field_t;


typedef struct _field
{
    enum types type;
    field_t value;
    char name[1024];
    int ip;

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
    SEP_COLOR   = 0X86B3B3,
    FUNC_NAME_COLOR = 0XFCBA03
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
