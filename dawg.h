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

typedef struct Dawg {
	FILE *file;
	uint32_t edge_count;
	uint32_t word_count;
	uint32_t node_count;
	int word_size;
} Dawg;

int dawg_init_file(Dawg *dawg, const char *filename);
void dawg_deinit(Dawg *dawg);

/**
 * Returns 1 for successful lookup, 0 if not in DAWG, -1 for I/O error.
 */
int dawg_lookup(Dawg *dawg, const char *string);
