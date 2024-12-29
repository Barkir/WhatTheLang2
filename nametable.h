#ifndef NAMETABLE_H
#define NAMETABLE_H

typedef struct _name
{
    const char * name;
    int address;
    struct _name * next;

} Name;

int InsertName(Name * names, Name name);
int EqualNames(Name name1, Name name2);
Name FindName(Name * names, Name name);



#endif
