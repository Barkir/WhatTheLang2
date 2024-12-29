#include <stdlib.h>
#include <string.h>
#include "nametable.h"


int InsertName(Name * names, Name name)
{
    if (EqualNames(*names, name)) return 0;
    if (!names)
    {
        names = malloc(sizeof(Name));
        *names = name;
        return 1;
    }

    InsertName(names->next, name);
    return 0;
}

int EqualNames(Name name1, Name name2)
{
    if (!strcmp(name1, name2) && name1.address == name2.address)
        return 1;
    return 0;
}

Name FindName(Name * names, Name name)
{
    if (EqualNames(name, *names))
        return *names;
    FindName(names->next, name);
    return NULL;
}
