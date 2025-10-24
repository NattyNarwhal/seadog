/*
 * Copyright (c) 2025 Calvin Buckley
 *
 * SPDX-License-Identifier: ISC
 */

#include <stdio.h>
#if __STDC_VERSION__ >= 199901L
# include <stdint.h>
#else
# include <limits.h>
/* for the C89 fans on decrepit architectures */
typedef unsigned char uint8_t;
# if UINT_MAX == 0xFFFFFFFF
typedef unsigned int uint32_t;
# elif ULONG_MAX == 0xFFFFFFFF
typedef unsigned long uint32_t;
# endif
# if INT_MAX == 0x7FFFFFFF
typedef int int32_t;
# elif LONG_MAX == 0x7FFFFFFF
typedef long int32_t;
# endif
#endif

/* GCC big endian definition */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DAWG_SWAP_WORD
#endif

#include "dawg.h"

#define DAWG_FIRST_INDEX 1
#define DAWG_FINAL (1 << 5)
#define DAWG_EOL (1 << 6)

#ifdef DAWG_SWAP_WORD
static inline uint32_t load32le(const uint8_t *src)
{
	return ((uint32_t)src[0] << 0)  | ((uint32_t)src[1] << 8)
	     | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}
#endif

static int dawg_lookup_index(Dawg *dawg, int32_t index, const char *string)
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
#ifdef DAWG_SWAP_WORD
		word = load32le((const uint8_t*)&word);
#endif
		index = word >> 7;
		character = (word & 0x1F) + 0x60; /* 26 -> 0x7A ('z') */
		if (string[0] == character && string[1] == '\0'
				&& (word & DAWG_FINAL)) {
			return 1;
		} else if (string[0] == character && index) {
			return dawg_lookup_index(dawg, index, string + 1);
		}
	} while ((word & DAWG_EOL) == 0);
	/* XXX: It'd be nice to return depth of no match for optimization */
	return 0;
}

int dawg_lookup(Dawg *dawg, const char *string)
{
	return dawg_lookup_index(dawg, DAWG_FIRST_INDEX, string);
}

int dawg_init_file(Dawg *dawg, const char *filename)
{
	dawg->file = fopen(filename, "rb");
	if (!dawg->file) {
		goto fail;
	}
	/* first byte (because LE) is DAWG_FINAL | DAWG_EOL | byte_size */
	dawg->word_size = fgetc(dawg->file) & 0x7; /* 2-4 */
	if (dawg->word_size == EOF || dawg->word_size > 4
			|| dawg->word_size < 2) {
		goto fail;
	}
	return 0;
fail:
	if (dawg->file) {
		fclose(dawg->file);
	}
	return -1;
}

void dawg_deinit(Dawg *dawg)
{
	if (dawg->file) {
		fclose(dawg->file);
	}
}
