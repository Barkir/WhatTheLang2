#ifndef PARSER_H
#define PARSER_H

#include "what_lang/tree.h"

static int DEF_SIZE = 1024;

enum colors
{
    OPER_COLOR  = 0XEFF94F,
    NUM_COLOR   = 0X5656EC,
    VAR_COLOR   = 0X70Df70,
    FUNC_EXT_COLOR  = 0X86E3E3,
    SEP_COLOR   = 0X86B3B3,
    FUNC_INTER_DEF_COLOR = 0XFCBA03,
    FUNC_INTER_CALL_COLOR = 0X7D3C98
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

Node * GetMajor(Node ** nodes, int * p);
Node * GetOperator(Node ** nodes, int * p);
Node * GetAssignment(Node ** nodes, int * p);
Node * GetExpression(Node ** nodes, int * p);
Node * GetTerm(Node ** nodes, int * p);
Node * GetPow(Node ** nodes, int * p);
Node * GetCompare(Node ** nodes, int * p);
Node * GetBracket(Node ** nodes, int * p);
Node * GetFuncExt(Node ** nodes, int * p);
Node * GetFuncInter(Node ** nodes, int * p);
Node * GetNumber(Node ** nodes, int * p);
Node * GetID(Node ** nodes, int * p);
Node * GetIf(Node ** nodes, int * p);
Node * GetWhile(Node ** nodes, int * p);
Node * GetGroup(Node ** nodes, int * p);
Node * GetParam(Node ** nodes, int * p);
Node * GetCall(Node ** nodes, int * p);



#endif
