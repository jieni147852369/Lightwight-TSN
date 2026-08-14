#ifndef HAVE_UPD6_H
#define HAVE_UPD6_H

#include "fd.h"
#include "transport.h"

/**
 * Allocate an instance of a UDP/IPv6 transport.
 * @return Pointer to a new transport instance on success, NULL otherwise.
 */
struct transport *udp6_transport_create(void);

#endif
