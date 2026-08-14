#ifndef HAVE_DM_H
#define HAVE_DM_H

/**
 * Defines the possible delay mechanisms.
 */
enum delay_mechanism {

	/** Start as E2E, but switch to P2P if a peer is detected. */
	DM_AUTO,

	/** Delay request-response mechanism. */
	DM_E2E,

	/** Peer delay mechanism. */
	DM_P2P,

	/** No Delay Mechanism. */
	DM_NO_MECHANISM = 0xFE,
};

#endif
