#ifndef HAVE_FAULT_H
#define HAVE_FAULT_H

#include <stdint.h>

enum fault_type {
	FT_UNSPECIFIED = 0,
	FT_BAD_PEER_NETWORK,
	FT_SWITCH_PHC,
	FT_CNT,
};

enum fault_tmo_type {
	FTMO_LINEAR_SECONDS = 0,
	FTMO_LOG2_SECONDS,
	FTMO_CNT,
};

struct fault_interval {
	enum fault_tmo_type type;
	int32_t val;
};

const char *ft_str(enum fault_type ft);

#endif
