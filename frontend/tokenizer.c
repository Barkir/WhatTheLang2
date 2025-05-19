#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "what_lang/errors.h"
#include "what_lang/tree.h"
#include "what_lang/parser.h"
#include "what_lang/tokenizer.h"

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


Node * _get_token(const char * string, int * p)
{
    SKIPSPACE;
    if (_is_oper(string, p)) {PARSER_LOG("operator %c! ", string[*p]); return _oper_token(string, p);}

    if (string[*p] == ';') {PARSER_LOG("separator of line!"); return _sep_token(string, p);}

    if (isalpha(string[*p])) {PARSER_LOG("name %c! ", string[*p]); return _name_token(string, p);}

    if (isdigit(string[*p])) {PARSER_LOG("number %c! ", string[*p]); return _number_token(string, p);}

    return NULL;

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
    if (strcmp(name, "<=") == 0)        return LESS_E;
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

Field _name_table(int name)
{
    Field ret = {.type = NUM, .value = -1};
    if (name < 0) return ret;
    for (int i = 0; i < DEF_SIZE; i++)
        if (NameTable[i].value == (field_t) name) return NameTable[i];
    return ret;
}



