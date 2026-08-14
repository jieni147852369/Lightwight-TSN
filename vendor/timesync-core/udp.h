#ifndef HAVE_UPD_H
#define HAVE_UPD_H

#include "fd.h"
#include "transport.h"

/**
 * Allocate an instance of a UDP/IPv4 transport.
 * @return Pointer to a new transport instance on success, NULL otherwise.
 */
struct transport *udp_transport_create(void);

#endif
