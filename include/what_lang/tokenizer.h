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
Node * _find_name       (char * result);



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

#endif
