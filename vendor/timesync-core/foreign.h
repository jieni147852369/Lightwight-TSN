#ifndef HAVE_FOREIGN_H
#define HAVE_FOREIGN_H

#include <sys/queue.h>

#include "ds.h"
#include "port.h"

#define FOREIGN_MASTER_THRESHOLD 2

struct foreign_clock {
	/**
	 * Pointer to next foreign_clock in list.
	 */
	LIST_ENTRY(foreign_clock) list;

	/**
	 * A list of received announce messages.
	 *
	 * The data set field, foreignMasterPortIdentity, is the
	 * sourcePortIdentity of the first message.
	 */
	TAILQ_HEAD(messages, ptp_message) messages;

	/**
	 * Number of elements in the message list,
	 * aka foreignMasterAnnounceMessages.
	 */
	unsigned int n_messages;

	/**
	 * Pointer to the associated port.
	 */
	struct port *port;

	/**
	 * Contains the information from the latest announce message
	 * in a form suitable for comparision in the BMCA.
	 */
	struct dataset dataset;
};

#endif
