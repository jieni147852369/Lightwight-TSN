#include "version.h"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

static const char *version = STRINGIFY(VER);

void version_show(FILE *fp)
{
	fprintf(fp, "%s\n", version);
}

const char *version_string(void)
{
	return version;
}
