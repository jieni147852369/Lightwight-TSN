#include <string.h>

#include "bmc.h"
#include "ds.h"

int telecom_dscmp(struct dataset *a, struct dataset *b)
{
	int diff;

	if (a == b)
		return 0;
	if (a && !b)
		return A_BETTER;
	if (b && !a)
		return B_BETTER;

	if (a->quality.clockClass < b->quality.clockClass)
		return A_BETTER;
	if (a->quality.clockClass > b->quality.clockClass)
		return B_BETTER;

	if (a->quality.clockAccuracy < b->quality.clockAccuracy)
		return A_BETTER;
	if (a->quality.clockAccuracy > b->quality.clockAccuracy)
		return B_BETTER;

	if (a->quality.offsetScaledLogVariance <
	    b->quality.offsetScaledLogVariance)
		return A_BETTER;
	if (a->quality.offsetScaledLogVariance >
	    b->quality.offsetScaledLogVariance)
		return B_BETTER;

	if (a->priority2 < b->priority2)
		return A_BETTER;
	if (a->priority2 > b->priority2)
		return B_BETTER;

	if (a->localPriority < b->localPriority)
		return A_BETTER;
	if (a->localPriority > b->localPriority)
		return B_BETTER;

	if (a->quality.clockClass <= 127)
		return dscmp2(a, b);

	diff = memcmp(&a->identity, &b->identity, sizeof(a->identity));

	if (!diff)
		return dscmp2(a, b);

	return diff < 0 ? A_BETTER : B_BETTER;
}
