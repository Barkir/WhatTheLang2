#ifndef CRC_32_H
#define CRC_32_H

unsigned int crc32_naive (const void* ptr, int len, unsigned int init);
unsigned int crc32_intinsic(const char* string);

#endif
