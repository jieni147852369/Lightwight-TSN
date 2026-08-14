#ifndef HAVE_FILTER_PRIVATE_H
#define HAVE_FILTER_PRIVATE_H

#include "tmv.h"
#include "contain.h"

struct filter {
	void (*destroy)(struct filter *filter);

	tmv_t (*sample)(struct filter *filter, tmv_t sample);

	void (*reset)(struct filter *filter);
};

#endif
