#ifndef NAMETABLE_H
#define NAMETABLE_H

const static size_t DEFAULT_NAME_ARRAY_SIZE = 32;

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

    int stack_offset;

    char ** adr_array;
    int adr_array_cap;

    struct _name ** name_array;            // Used to contain variables

} Name;

typedef struct FuncInfo {
    const char *func_name;   // Имя функции (ноды)
    char **offsets;          // Массив адресов (смещений в буфере)
    size_t offsets_count;     // Количество элементов в offsets
    size_t offsets_capacity; // Выделенная память под offsets
} FuncInfo;

int InsertName(Name * names, Name name);
int EqualNames(Name name1, Name name2);
Name FindName(Name * names, Name name);





#endif
