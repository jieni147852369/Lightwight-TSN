#ifndef HAVE_NOTIFICATION_H
#define HAVE_NOTIFICATION_H

#include <stdbool.h>
#include <stdint.h>

static inline void event_bitmask_set(uint8_t *bitmask, unsigned int event,
				     bool value)
{
	unsigned int event_pos = event / 8;
	uint8_t event_bit = 1 << (event % 8);

	if (value) {
		bitmask[event_pos] |= event_bit;
	} else {
		bitmask[event_pos] &= ~(event_bit);
	}
}

static inline bool event_bitmask_get(uint8_t *bitmask, unsigned int event)
{
	return (bitmask[event / 8] & (1 << (event % 8))) ? true : false;
}

enum notification {
	NOTIFY_PORT_STATE,
	NOTIFY_TIME_SYNC,
	NOTIFY_PARENT_DATA_SET,
};

#endif
