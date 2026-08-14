#ifndef HAVE_UDS_H
#define HAVE_UDS_H

#include "config.h"
#include "fd.h"
#include "transport.h"

/**
 * Allocate an instance of a UDS transport.
 * @return Pointer to a new transport instance on success, NULL otherwise.
 */
struct transport *uds_transport_create(void);

#endif
