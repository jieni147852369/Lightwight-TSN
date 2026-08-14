#ifndef HAVE_ETHER_H
#define HAVE_ETHER_H

#include <stdint.h>

#define EUI48 6
#define EUI64 8

#define MAC_LEN  EUI48
#define GUID_LEN EUI64

#define GUID_OFFSET 36

typedef uint8_t eth_addr[MAC_LEN];

struct eth_hdr {
	eth_addr dst;
	eth_addr src;
	uint16_t type;
} __attribute__((packed));

#define VLAN_HLEN 4

struct vlan_hdr {
	eth_addr dst;
	eth_addr src;
	uint16_t tpid;
	uint16_t tci;
	uint16_t type;
} __attribute__((packed));

#endif
