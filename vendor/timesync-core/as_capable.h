#ifndef HAVE_AS_CAPABLE_H
#define HAVE_AS_CAPABLE_H

/* Enum used by the asCapable config option. */
enum as_capable_option {
	AS_CAPABLE_TRUE,
	AS_CAPABLE_AUTO,
};

/*
 * Defines whether the device can interoperate with the device on other end via
 * IEEE 802.1AS protocol.
 *
 * More information about this in Section 10.2.4.1 of IEEE 802.1AS standard.
 */
enum as_capable {
	NOT_CAPABLE,
	AS_CAPABLE,
	/*
	 * Non-standard extension to support Automotive Profile. asCapable
	 * always set to true without checking the system at other end.
	 */
	ALWAYS_CAPABLE,
};

#endif
