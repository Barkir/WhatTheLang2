#ifndef TOKENIZER_H
#define TOKENIZER_H

#define SKIPSPACE while(isspace(string[*p]) || (string[*p]) == ',') (*p)++


const char * _enum_to_name(int name);
int _name_to_enum(const char * name);

Field _name_table       (int name);
int _is_oper            (const char * string, int * p);
Node ** StringTokenize  (const char * string, int * p);
Node * _get_token       (const char * string, int * p);
Node * _sep_token       (const char * string, int * p);
Node * _number_token    (const char * string, int * p);
Node * _name_token      (const char * string, int * p);
Node * _oper_token      (const char * string, int * p);
Node * _find_name       (char * result, int is_func);

static Field NameTable[] =
{
    {.type = FUNC_EXT,  .value = SIN    },
    {.type = FUNC_EXT,  .value = COS    },
    {.type = FUNC_EXT,  .value = TG     },
    {.type = FUNC_EXT,  .value = CTG    },
    {.type = FUNC_EXT,  .value = SH     },
    {.type = FUNC_EXT,  .value = CH     },
    {.type = FUNC_EXT,  .value = TH     },
    {.type = FUNC_EXT,  .value = CTH    },
    {.type = FUNC_EXT,  .value = LN     },
    {.type = FUNC_EXT,  .value = LOG    },
    {.type = FUNC_EXT,  .value = PRINT  },
    {.type = FUNC_EXT,  .value = SQRT   },
    {.type = FUNC_EXT,  .value = INPUT  },
    {.type = OPER    ,  .value = IF     },
    {.type = OPER    ,  .value = WHILE  },
    {.type = OPER    ,  .value = MORE   },
    {.type = OPER    ,  .value = LESS   },
    {.type = OPER    ,  .value = MORE_E },
    {.type = OPER    ,  .value = LESS_E },
    {.type = OPER    ,  .value = EQUAL  },
    {.type = OPER    ,  .value = N_EQUAL},
    {.type = OPER    ,  .value = DEF    },
};

typedef struct _token_map
{
    int token_enum;
    const char * token_str;
    const char * graphviz_str

}  TokenMap;

#define TOKEN_ARRAY_SIZE 256

#define DEF_TOKEN_LINE(enum_name, str, graphviz)                                                            \
    [enum_name] = {(enum_name), (str), (graphviz)}

static TokenMap TokenArray[TOKEN_ARRAY_SIZE] =
{
    DEF_TOKEN_LINE(SQRT,    "SQRT",  "sqrt"    ),
    DEF_TOKEN_LINE(SIN,     "sin",   "sin"     ),
    DEF_TOKEN_LINE(COS,     "cos",   "cos"     ),
    DEF_TOKEN_LINE(TG,      "tg",    "tg"      ),
    DEF_TOKEN_LINE(CTG,     "ctg",   "ctg"     ),
    DEF_TOKEN_LINE(SH,      "sh",    "sh"      ),
    DEF_TOKEN_LINE(CH,      "ch",    "ch"      ),
    DEF_TOKEN_LINE(LN,      "LOG",   "ln"      ),
    DEF_TOKEN_LINE(IF,      "if",    "if"      ),
    DEF_TOKEN_LINE(WHILE,   "while", "while"   ),
    DEF_TOKEN_LINE(PRINT,   "print", "print"   ),
    DEF_TOKEN_LINE(DEF,     "def",   "def"     ),
    DEF_TOKEN_LINE(INPUT,   "input", "input"   ),
    DEF_TOKEN_LINE(MORE,    ">",     "more"    ),
    DEF_TOKEN_LINE(MORE_E,  ">=",    "more_e"  ),
    DEF_TOKEN_LINE(LESS,    "<",     "less"    ),
    DEF_TOKEN_LINE(LESS_E,  "<=",    "less_e"  ),
    DEF_TOKEN_LINE(EQUAL,   "==",    "equal"   ),
    DEF_TOKEN_LINE(N_EQUAL, "!=",    "n_equal" ),
    DEF_TOKEN_LINE('+',     "+",     "+"       ),
    DEF_TOKEN_LINE('-',     "-",     "-"       ),
    DEF_TOKEN_LINE('*',     "*",     "*"       ),
    DEF_TOKEN_LINE('/',     "/",     "//"      ),
    DEF_TOKEN_LINE('^',     "^",     "^"       ),
    DEF_TOKEN_LINE('=',     "=",     "="       ),
    DEF_TOKEN_LINE('(',     "(",     "("       ),
    DEF_TOKEN_LINE(')',     ")",     ")"       ),
    DEF_TOKEN_LINE('{',     "{",     "{"       ),
    DEF_TOKEN_LINE('}',     "}",     "}"       )
};

#endif
