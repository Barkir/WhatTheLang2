#ifndef TOKENIZER_H
#define TOKENIZER_H

#define SKIPSPACE while(isspace(string[*p]) || (string[*p]) == ',') (*p)++


const char * _enum_to_name(int name);
int _name_to_enum       (char * name);

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

#define TOKEN_ARRAY_SIZE 29

static TokenMap TokenArray[TOKEN_ARRAY_SIZE] =
{
    {.token_enum = SQRT   , .token_str = "SQRT" , .graphviz_str = "sqrt"    },
    {.token_enum = SIN    , .token_str = "sin"  , .graphviz_str = "sin"     },
    {.token_enum = COS    , .token_str = "cos"  , .graphviz_str = "cos"     },
    {.token_enum = TG     , .token_str = "tg"   , .graphviz_str = "tg"      },
    {.token_enum = CTG    , .token_str = "ctg"  , .graphviz_str = "ctg"     },
    {.token_enum = SH     , .token_str = "sh"   , .graphviz_str = "sh"      },
    {.token_enum = CH     , .token_str = "ch"   , .graphviz_str = "ch"      },
    {.token_enum = LN     , .token_str = "LOG"  , .graphviz_str = "ln"      },
    {.token_enum = IF     , .token_str = "if"   , .graphviz_str = "if"      },
    {.token_enum = WHILE  , .token_str = "while", .graphviz_str = "while"   },
    {.token_enum = PRINT  , .token_str = "print", .graphviz_str = "print"   },
    {.token_enum = DEF    , .token_str = "def"  , .graphviz_str = "def"     },
    {.token_enum = INPUT  , .token_str = "input", .graphviz_str = "input"   },
    {.token_enum = MORE   , .token_str = ">"    , .graphviz_str = "more"    },
    {.token_enum = MORE_E , .token_str = ">="   , .graphviz_str = "more_e"  },
    {.token_enum = LESS   , .token_str = "<"    , .graphviz_str = "less"    },
    {.token_enum = LESS_E , .token_str = "<="   , .graphviz_str = "less_e"  },
    {.token_enum = EQUAL  , .token_str = "=="   , .graphviz_str = "equal"   },
    {.token_enum = N_EQUAL, .token_str = "!="   , .graphviz_str = "n_equal" },
    {.token_enum = '+'    , .token_str = "+"    , .graphviz_str = "+"       },
    {.token_enum = '-'    , .token_str = "-"    , .graphviz_str = "-"       },
    {.token_enum = '*'    , .token_str = "*"    , .graphviz_str = "*"       },
    {.token_enum = '/'    , .token_str = "/"    , .graphviz_str = "//"      },
    {.token_enum = '^'    , .token_str = "^"    , .graphviz_str = "^"       },
    {.token_enum = '='    , .token_str = "="    , .graphviz_str = "="      },
    {.token_enum = '('    , .token_str = "("    , .graphviz_str = "("       },
    {.token_enum = ')'    , .token_str = ")"    , .graphviz_str = ")"       },
    {.token_enum = '{'    , .token_str = "{"    , .graphviz_str = "{"       },
    {.token_enum = '}'    , .token_str = "}"    , .graphviz_str = "}"       }
};

#endif
