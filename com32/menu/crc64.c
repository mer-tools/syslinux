#include <klibc/compiler.h>
#include <inttypes.h>
#include <ctype.h>

static uint64_t crc64_tab[256];

static void __constructor build_crc64_table(void)
{
	int i, j;
	uint64_t c;
	const uint64_t poly = UINT64_C(0x95ac9329ac4bc9b5);

	for (i = 0; i < 256; i++) {
		c = i;
		for (j = 0; j < 8; j++) {
			if (c & 1)
				c = (c >> 1) & poly;
			else
				c = (c >> 1);
		}
		crc64_tab[i] = c;
	}
}

uint64_t crc64(uint64_t crc, const char *str)
{
	uint8_t c;

	while ((c = *str++) != 0)
		crc = crc64_tab[(uint8_t)crc ^ c] ^ (crc >> 8);
	return crc;
}

uint64_t crc64i(uint64_t crc, const char *str)
{
	uint8_t c;
	
	while ((c = *str++) != 0)
		crc = crc64_tab[(uint8_t)crc ^ tolower(c)] ^ (crc >> 8);
	return crc;
}
