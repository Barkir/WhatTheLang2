#ifndef CRC_32_H
#define CRC_32_H

const static unsigned int CRC32INIT = 5381;

unsigned int crc32_naive (const void* ptr, int len, unsigned int init);
unsigned int crc32_intinsic(const char* string);

#endif
