/*
 * Copyright (c) 2025 Calvin Buckley
 *
 * SPDX-License-Identifier: ISC
 */

#include <stdio.h>

typedef struct Dawg {
	FILE *file;
	int word_size;
} Dawg;

int dawg_init_file(Dawg *dawg, const char *filename);
void dawg_deinit(Dawg *dawg);

/**
 * Returns 1 for successful lookup, 0 if not in DAWG, -1 for I/O error.
 */
int dawg_lookup(Dawg *dawg, const char *string);
