#include <stdio.h>
#include <stdint.h>

typedef struct Dawg {
	FILE *f;
	int s;
} Dawg;

#define DAWG_FINAL (1 << 5)
#define DAWG_EOL (1 << 6)

int lookup(Dawg *d, int i, const char *s)
{
	int w; /* [iiii_iiii...]_iefc_cccc (2-4 bytes) */
	char c;
	const char *n;
	n = s + 1;
	if (0 != fseek(d->f, i * d->s, SEEK_SET)) {
		return -1;
	}
	do {
		w = 0; /* clear high bytes for compact */
		if (1 == fread(&w, 1, d->s, d->f)) {
			return -1;
		}
		/* (swap on big) */
		i = w >> 7;
		c = (w & 0x1F) + 0x60; /* 26 -> 0x7A ('z') */
		if (*s == c && *n == '\0' && (w & DAWG_FINAL)) {
			return 1;
		} else if (*s == c && i) {
			return lookup(d, i, n);
		}
	} while ((w & DAWG_EOL) == 0);
	return 0;
}

int main(int argc, char **argv)
{
	Dawg d;
	int i;
	d.f = fopen(argv[1], "rb");
	d.s = fgetc(d.f) & 0x7; /* 2-4 */
	for (i = 2; i < argc; i++) {
		printf("%s? %d\n", argv[i], lookup(&d, 1, argv[i]));
	}
	return 0;
}
