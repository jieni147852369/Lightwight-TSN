#ifndef HAVE_TS2PHC_PPS_SOURCE_PRIVATE_H
#define HAVE_TS2PHC_PPS_SOURCE_PRIVATE_H

#include <stdint.h>
#include <time.h>

#include "contain.h"
#include "ts2phc_pps_source.h"

struct ts2phc_pps_source {
	void (*destroy)(struct ts2phc_pps_source *src);
	int (*getppstime)(struct ts2phc_pps_source *src, struct timespec *ts);
	struct ts2phc_clock *(*get_clock)(struct ts2phc_pps_source *src);
};

#endif
