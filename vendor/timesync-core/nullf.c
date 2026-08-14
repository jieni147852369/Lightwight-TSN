#include <stdlib.h>
#include <math.h>

#include "nullf.h"
#include "print.h"
#include "servo_private.h"

struct nullf_servo {
	struct servo servo;
};

static void nullf_destroy(struct servo *servo)
{
	struct nullf_servo *s = container_of(servo, struct nullf_servo, servo);
	free(s);
}

static double nullf_sample(struct servo *servo, int64_t offset,
			   uint64_t local_ts, double weight,
			   enum servo_state *state)
{
	long long int abs_offset = llabs(offset);

	if ((servo->offset_threshold && abs_offset < servo->offset_threshold) ||
	    (servo->step_threshold && servo->step_threshold >= abs_offset)) {
		*state = SERVO_LOCKED;
		return 0.0;
	}

	if ((servo->first_update && servo->first_step_threshold &&
	     servo->first_step_threshold < abs_offset) ||
	    (servo->step_threshold && servo->step_threshold < abs_offset)) {
		*state = SERVO_JUMP;
	} else {
		*state = SERVO_UNLOCKED;
	}

	return 0.0;
}

static void nullf_sync_interval(struct servo *servo, double interval)
{
}

static void nullf_reset(struct servo *servo)
{
}

struct servo *nullf_servo_create(void)
{
	struct nullf_servo *s;

	s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;

	s->servo.destroy = nullf_destroy;
	s->servo.sample = nullf_sample;
	s->servo.sync_interval = nullf_sync_interval;
	s->servo.reset = nullf_reset;

	return &s->servo;
}
