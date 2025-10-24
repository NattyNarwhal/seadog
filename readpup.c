#include <stdio.h>

#include "dawg.h"

int main(int argc, char **argv)
{
	Dawg dawg;
	int i;
	if (argc < 2) {
		return 1;
	}
	dawg_init_file(&dawg, argv[1]);
	for (i = 2; i < argc; i++) {
		printf("%s? %d\n", argv[i], dawg_lookup(&dawg, argv[i]));
	}
	dawg_deinit(&dawg);
	return 0;
}
