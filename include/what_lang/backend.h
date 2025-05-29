#ifndef BACKEND_H
#define BACKEND_H

const static size_t LABEL_SIZE = 32;
const static size_t ELF_HEADER_SIZE = 64;

int CreateAsm(Tree * tree, const char * filename);

int _create_asm(Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count);
int _create_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count);

int _def_bin(char ** buf, Htable ** tab, Name * names, Node * root, FILE * file, int if_cond, int while_cond, int if_count, int while_count);

static int IF_COUNT = 0;
static int WHILE_COUNT = 0;
static int ADR_COUNT = 0;

enum Registers
{
    WHAT_REG_EAX = 0x00,
    WHAT_REG_ECX = 0x01,
    WHAT_REG_EDX = 0x02,
    WHAT_REG_EBX = 0x03,
    WHAT_REG_ESP = 0x04,
    WHAT_REG_EBP = 0x05,
    WHAT_REG_ESI = 0x06,
    WHAT_REG_EDI = 0x07,
};

enum AdditionalRegisters
{
    WHAT_REG_R8  = 0x00,
    WHAT_REG_R9  = 0x01,
    WHAT_REG_R10 = 0x02,
    WHAT_REG_R11 = 0x03,
    WHAT_REG_R12 = 0x04,
    WHAT_REG_R13 = 0x05,
    WHAT_REG_R14 = 0x06,
    WHAT_REG_R15 = 0x07,
};

enum RunModes
{
    WHAT_DEBUG_MODE,
    WHAT_NASM_MODE,
    WHAT_BIN_MODE
};

int CreateBin(Tree * tree, const char * filename_asm, const char * filename_bin, enum RunModes mode);

// typedef struct _locals
// {
//     char * if_label;
//     char * if_end_label;
//     char * while_false_label;
//     char * while_true_label;
//     char * while_end_label;
//
// } Locals;
//
// typedef struct _ctx
// {
//     static int ctx_if_count;
//     static int ctx_while_count;
//     static int ctx_adr_count;
//
//     static int if_cond;
//     static int while_cond;
//
// } Ctx;


// static struct _ctx GlobalContext =
// {
//     .ctx_adr_count      = 0,
//     .ctx_if_count       = 0,
//     .ctx_while_count    = 0
// };


#endif
