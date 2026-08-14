#include "fault.h"

static const char *fault_type_str[FT_CNT] = {
	"FT_UNSPECIFIED",
	"FT_BAD_PEER_NETWORK",
	"FT_SWITCH_PHC",
};

const char *ft_str(enum fault_type ft)
{
	if (ft < 0 || ft >= FT_CNT)
		return "INVALID_FAULT_TYPE_ENUM";
	return fault_type_str[ft];
}
