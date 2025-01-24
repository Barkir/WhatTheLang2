#ifndef NAMETABLE_H
#define NAMETABLE_H

typedef struct _name
{
    const char * name;
    const char * func_name;

    int address;
    int address_end;
    int type;
    int param;

} Name;

int InsertName(Name * names, Name name);
int EqualNames(Name name1, Name name2);
Name FindName(Name * names, Name name);



#endif
