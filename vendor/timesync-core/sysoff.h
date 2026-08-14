#include <stdint.h>
#include "missing.h"

enum {
	SYSOFF_RUN_TIME_MISSING = -1,
	SYSOFF_PRECISE,
	SYSOFF_EXTENDED,
	SYSOFF_BASIC,
	SYSOFF_LAST,
};

/**
 * Check to see if a PTP_SYS_OFFSET ioctl is supported.
 * @param fd  An open file descriptor to a PHC device.
 * @return  One of the SYSOFF_ enumeration values.
 */
int sysoff_probe(int fd, int n_samples);

/**
 * Measure the offset between a PHC and the system time.
 * @param fd         An open file descriptor to a PHC device.
 * @param method     A non-negative SYSOFF_ value returned by sysoff_probe().
 * @param n_samples  The number of consecutive readings to make.
 * @param result     The estimated offset in nanoseconds.
 * @param ts         The system time corresponding to the 'result'.
 * @param delay      The delay in reading of the clock in nanoseconds.
 * @return  Zero on success, negative error code otherwise.
 */
int sysoff_measure(int fd, int method, int n_samples,
		   int64_t *result, uint64_t *ts, int64_t *delay);
