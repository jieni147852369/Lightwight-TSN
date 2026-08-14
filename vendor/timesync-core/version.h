#ifndef HAVE_VERSION_H
#define HAVE_VERSION_H

#include <stdio.h>

/**
 * Print the software version string into the given file.
 * @param fp  File pointer open for writing.
 */
void version_show(FILE *fp);

/**
 * Provide the software version as a human readable string.
 * @return  Pointer to a static global buffer holding the result.
 */
const char *version_string(void);

#endif
