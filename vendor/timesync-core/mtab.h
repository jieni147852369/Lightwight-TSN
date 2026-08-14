#ifndef HAVE_MTAB_H
#define HAVE_MTAB_H

#include <sys/queue.h>
#include <time.h>

#include "address.h"
#include "pdt.h"
#include "transport.h"
#include "unicast_fsm.h"

struct unicast_master_address {
	STAILQ_ENTRY(unicast_master_address) list;
	struct PortIdentity portIdentity;
	enum transport_type type;
	enum unicast_state state;
	struct address address;
	unsigned int granted;
	unsigned int sydymsk;
	time_t renewal_tmo;
};

struct unicast_master_table {
	STAILQ_HEAD(addrs_head, unicast_master_address) addrs;
	STAILQ_ENTRY(unicast_master_table) list;
	Integer8 logQueryInterval;
	int table_index;
	int count;
	int port;
	/* for use with P2P delay mechanism: */
	struct unicast_master_address peer_addr;
	char *peer_name;
};

#endif
