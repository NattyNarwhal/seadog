#include <stdio.h>

#include "dawg.h"

int main(int argc, char **argv)
{
	Dawg dawg;
	int i;
	if (argc < 2) {
		return 1;
	}
	init_dawg_file(&dawg, argv[1]);
	for (i = 2; i < argc; i++) {
		printf("%s? %d\n", argv[i], dawg_lookup(&dawg, argv[i]));
	}
	deinit_dawg(&dawg);
	return 0;
}
