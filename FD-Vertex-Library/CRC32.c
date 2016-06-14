#include "CRC32.h"

unsigned int crc32(const unsigned char *buf, unsigned int size)
{
	unsigned int crc = 0;

	for (unsigned int n = 0; n < size; n++) {
		crc = UPDC32(buf[n], crc);
	}

	return crc;
}

