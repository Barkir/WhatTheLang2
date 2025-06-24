#ifndef IO_H
#define IO_H

enum IO_FLAGS
{
    IO_ALIGNED,
    IO_DEFAULT
};

int ProcessCmd(int argc, char * argv[]);
int File2Lines(int a_flag, char *** buf, const char * filename);
#endif
