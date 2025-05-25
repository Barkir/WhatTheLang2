#ifndef NAMETABLE_H
#define NAMETABLE_H

typedef struct _name
{

    const char * name;              // Name of variable
    const char * func_name;         // Name of variable's function



    char * local_func_name;         // This part is used for processing labels in backend (sorry, shouldn't be here)
    char * offset;                  // Label offset (used to set the jmp byte)

    int address;                    // Parameters for counting variables of the function
    int address_end;                // They're placed one by one and we need to know the address of the first one to know the addresses of other one's
    int type;
    int param;

    int is_in_reg;                  // Shows if variable is located in register or in stack
    int location;                   // Register enum or rsp value

} Name;

int InsertName(Name * names, Name name);
int EqualNames(Name name1, Name name2);
Name FindName(Name * names, Name name);





#endif
