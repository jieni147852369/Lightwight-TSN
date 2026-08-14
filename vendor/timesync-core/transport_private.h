#ifndef HAVE_TRANSPORT_PRIVATE_H
#define HAVE_TRANSPORT_PRIVATE_H

#include <time.h>

#include "address.h"
#include "fd.h"
#include "transport.h"

struct transport {
	enum transport_type type;
	struct config *cfg;

	int (*close)(struct transport *t, struct fdarray *fda);

	int (*open)(struct transport *t, struct interface *iface,
		    struct fdarray *fda, enum timestamp_type tt);

	int (*recv)(struct transport *t, int fd, void *buf, int buflen,
		    struct address *addr, struct hw_timestamp *hwts);

	int (*send)(struct transport *t, struct fdarray *fda,
		    enum transport_event event, int peer, void *buf, int buflen,
		    struct address *addr, struct hw_timestamp *hwts);

	void (*release)(struct transport *t);

	int (*physical_addr)(struct transport *t, uint8_t *addr);

	int (*protocol_addr)(struct transport *t, uint8_t *addr);
};

#endif
