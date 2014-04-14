#include "perror.h"

#include <stdio.h>
#include <git2.h>

void gbr_perror(const char *prefix)
{
	fprintf(stderr, "%s: %s\n", prefix, giterr_last()->message);
}

