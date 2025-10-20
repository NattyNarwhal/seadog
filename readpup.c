#include <stdio.h>
#include <stdint.h>

typedef struct Dawg {
	FILE *file;
	int word_size;
} Dawg;

#define DAWG_FINAL (1 << 5)
#define DAWG_EOL (1 << 6)

int lookup(Dawg *dawg, int index, const char *string)
{
	int word; /* [iiii_iiii...]_iefc_cccc (2-4 bytes) */
	char character;
	if (0 != fseek(dawg->file, index * dawg->word_size, SEEK_SET)) {
		return -1;
	}
	do {
		word = 0; /* clear high bytes for compact */
		if (1 == fread(&word, 1, dawg->word_size, dawg->file)) {
			return -1;
		}
		/* (swap on big) */
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

int main(int argc, char **argv)
{
	Dawg dawg;
	int i;
	init_dawg_file(&dawg, argv[1]);
	for (i = 2; i < argc; i++) {
		printf("%s? %d\n", argv[i], lookup(&dawg, 1, argv[i]));
	}
	return 0;
}
