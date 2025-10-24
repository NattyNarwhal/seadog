#include <stdio.h>

#include "dawg.h"

int main(int argc, char **argv)
{
	Dawg dawg;
	if (argc < 2) {
		return 1;
	}
	init_dawg_file(&dawg, argv[1]);
	dawg_dump(&dawg);
	deinit_dawg(&dawg);
	return 0;
}
