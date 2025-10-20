#include <stdio.h>
#if __STDC_VERSION__ >= 199901L
# include <stdint.h>
#else
/* for the C89 fans on decrepit architectures */
# if UINT_MAX == 0xFFFFFFFF
typedef unsigned int uint32_t;
# elif ULONG_MAX == 0xFFFFFFFF
typedef unsigned long uint32_t;
# endif
#endif

typedef struct Dawg {
	FILE *file;
	int word_size;
} Dawg;

#define DAWG_FINAL (1 << 5)
#define DAWG_EOL (1 << 6)

static inline uint32_t load32le(const unsigned char *src)
{
	return ((uint32_t)src[0] << 0) | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) |
	       ((uint32_t)src[3] << 24);
}

int lookup(Dawg *dawg, int index, const char *string)
{
	uint32_t word; /* [iiii_iiii...]_iefc_cccc (2-4 bytes) */
	char character;
	if (0 != fseek(dawg->file, index * dawg->word_size, SEEK_SET)) {
		return -1;
	}
	do {
		word = 0; /* clear high bytes for compact */
		if (1 == fread(&word, 1, dawg->word_size, dawg->file)) {
			return -1;
		}
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		word = load32le((const unsigned char*)&word);
#endif
		index = word >> 7;
		character = (word & 0x1F) + 0x60; /* 26 -> 0x7A ('z') */
		if (string[0] == character && string[1] == '\0' && (word & DAWG_FINAL)) {
			return 1;
		} else if (string[0] == character && index) {
			return lookup(dawg, index, string + 1);
		}
	} while ((word & DAWG_EOL) == 0);
	return 0;
}

int init_dawg_file(Dawg *dawg, const char *filename)
{
	dawg->file = fopen(filename, "rb");
	if (!dawg->file) {
		goto fail;
	}
	dawg->word_size = fgetc(dawg->file) & 0x7; /* 2-4 */
	if (dawg->word_size == EOF) {
		goto fail;
	}
	return 0;
fail:
	if (dawg->file) {
		fclose(dawg->file);
	}
	return -1;
}

void deinit_dawg(Dawg *dawg)
{
	if (dawg->file) {
		fclose(dawg->file);
	}
}

int main(int argc, char **argv)
{
	Dawg dawg;
	int i;
	init_dawg_file(&dawg, argv[1]);
	for (i = 2; i < argc; i++) {
		printf("%s? %d\n", argv[i], lookup(&dawg, 1, argv[i]));
	}
	deinit_dawg(&dawg);
	return 0;
}
