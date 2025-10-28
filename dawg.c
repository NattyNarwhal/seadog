/*
 * Copyright (c) 2025 Calvin Buckley
 *
 * SPDX-License-Identifier: ISC
 */

#include "dawg.h"

/* GCC big endian definition */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DAWG_SWAP_WORD
#endif

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

static uint32_t dawg_read_word(Dawg *dawg)
{
	uint32_t word;
	word = 0; /* clear high bytes for compact */
	if (1 != fread(&word, dawg->word_size, 1, dawg->file)) {
		return 0;
	}
#ifdef DAWG_SWAP_WORD
	word = load32le((const uint8_t*)&word);
#endif
	return word;
}

static int dawg_lookup_index(Dawg *dawg, int32_t index, const char *string)
{
	uint32_t word; /* [iiii_iiii...]_iefc_cccc (2-4 bytes) */
	char character;
	if (!index || 0 != fseek(dawg->file, index * dawg->word_size, SEEK_SET)) {
		return -1;
	}
	do {
		word = dawg_read_word(dawg);
		if (0 == word) {
			return -1;
		}
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
	int word_size; /* temporary */
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
	/* now that we know byte size, read the whole word for index */
	rewind(dawg->file);
	dawg->edge_count = dawg_read_word(dawg);
	if (0 == dawg->edge_count) {
		goto fail;
	}
	dawg->edge_count >>= 7;
	/* check if EOL bit is set in last index node (sanity check) */
	if (-1 == fseek(dawg->file, dawg->edge_count * dawg->word_size, SEEK_SET)) {
		goto fail;
	}
	if (0 == (dawg_read_word(dawg) & DAWG_EOL)) {
		goto fail;
	}
	/* actual last words in file for word and edge count, always 4 bytes */
	/* swap b/c it's likely faster than adding another branch to read */
	word_size = dawg->word_size;
	dawg->word_size = 4;
	dawg->word_count = dawg_read_word(dawg);
	dawg->node_count = dawg_read_word(dawg);
	if (0 == dawg->word_count || 0 == dawg->node_count) {
		goto fail;
	}
	dawg->word_size = word_size;
	return 0;
fail:
	dawg_deinit(dawg);
	return -1;
}

void dawg_deinit(Dawg *dawg)
{
	if (dawg->file) {
		fclose(dawg->file);
		dawg->file = NULL;
	}
}
