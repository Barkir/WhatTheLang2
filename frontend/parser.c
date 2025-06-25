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

#include "what_lang/constants.h"
#include "what_lang/buff.h"
#include "what_lang/parser.h"
#include "what_lang/nametable.h"
#include "what_lang/errors.h"
#include "what_lang/tokenizer.h"

const char * _enum_to_name(int name);

void SyntaxError(char exp, char real, const char * func, int line)
{
    assert(func);

    fprintf(stderr, ">>> SyntaxError %s %d: <expected %c> <got %c (%lg)>", func, line, exp, (int) real, (field_t) real);
    assert(0);
}

Node * GetMajor(Node ** nodes, int * p)
{
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

    int old_p = (*p);
    if (!nodes[*p]) return NULL;
    PARSER_LOG("Getting O... Got node %p with val %lg, type %d name %s, p = %d", nodes[*p], NodeValue(nodes[*p]), NodeType(nodes[*p]), NodeName(nodes[*p]), *p);
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

    if (val1 = GetFuncExt(nodes, p))
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

    if (val1 = GetFuncInter(nodes, p))
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
    assert(nodes);
    assert(p);

    if ((int) NodeValue(nodes[*p]) != DEF) return NULL;
    Field * field = _copy_field(((Field*)((char*)(nodes[*p]) + sizeof(Node))));
    PARSER_LOG("Getting CALL... p = %d", *p);
    (*p)++;
    Node * name = GetID(nodes, p);
    ((Field*)((char*)name + sizeof(Node)))->type = FUNC_INTER_DEF;

    Node * group = GetGroup(nodes, p);
    return _create_node(field, name, group);
}

Node * GetGroup(Node ** nodes, int * p)
{
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

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

    if (NodeType(nodes[*p]) == FUNC_INTER_CALL) return GetFuncInter(nodes, p);
    else if (NodeType(nodes[*p]) == FUNC_EXT) return GetFuncExt(nodes, p);
    else if (NodeType(nodes[*p]) == VAR)  return GetID(nodes, p);
    else return GetNumber(nodes, p);
}

Node * GetFuncInter(Node ** nodes, int * p)
{
    assert(nodes);
    assert(p);

    if (NodeType(nodes[*p]) != FUNC_INTER_CALL && NodeType(nodes[*p]) != VAR) return NULL;
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

Node * GetFuncExt(Node ** nodes, int * p)
{
    assert(nodes);
    assert(p);

    if ((int) NodeType(nodes[*p]) != FUNC_EXT) return NULL;
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
    assert(nodes);
    assert(p);

    if (!nodes[*p]) return NULL;
    PARSER_LOG("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

Node * GetParam(Node ** nodes, int * p)
{
    assert(nodes);
    assert(p);

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
    assert(nodes);
    assert(p);

    if (NodeType(nodes[*p]) != VAR && NodeType(nodes[*p]) != FUNC_INTER_CALL) return NULL;
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



