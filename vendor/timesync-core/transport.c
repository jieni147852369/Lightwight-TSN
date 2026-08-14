#include <arpa/inet.h>

#include "transport.h"
#include "transport_private.h"
#include "raw.h"
#include "udp.h"
#include "udp6.h"
#include "uds.h"

int transport_close(struct transport *t, struct fdarray *fda)
{
	return t->close(t, fda);
}

int transport_open(struct transport *t, struct interface *iface,
		   struct fdarray *fda, enum timestamp_type tt)
{
	return t->open(t, iface, fda, tt);
}

int transport_recv(struct transport *t, int fd, struct ptp_message *msg)
{
	return t->recv(t, fd, msg, sizeof(msg->data), &msg->address, &msg->hwts);
}

int transport_send(struct transport *t, struct fdarray *fda,
		   enum transport_event event, struct ptp_message *msg)
{
	int len = ntohs(msg->header.messageLength);

	return t->send(t, fda, event, 0, msg, len, NULL, &msg->hwts);
}

int transport_peer(struct transport *t, struct fdarray *fda,
		   enum transport_event event, struct ptp_message *msg)
{
	int len = ntohs(msg->header.messageLength);

	return t->send(t, fda, event, 1, msg, len, NULL, &msg->hwts);
}

int transport_sendto(struct transport *t, struct fdarray *fda,
		     enum transport_event event, struct ptp_message *msg)
{
	int len = ntohs(msg->header.messageLength);

	return t->send(t, fda, event, 0, msg, len, &msg->address, &msg->hwts);
}

int transport_txts(struct fdarray *fda,
		   struct ptp_message *msg)
{
	int cnt, len = ntohs(msg->header.messageLength);
	struct hw_timestamp *hwts = &msg->hwts;
	unsigned char pkt[1600];

	cnt = sk_receive(fda->fd[FD_EVENT], pkt, len, NULL, hwts, MSG_ERRQUEUE);
	return cnt > 0 ? 0 : cnt;
}

int transport_physical_addr(struct transport *t, uint8_t *addr)
{
	if (t->physical_addr) {
		return t->physical_addr(t, addr);
	}
	return 0;
}

int transport_protocol_addr(struct transport *t, uint8_t *addr)
{
	if (t->protocol_addr) {
		return t->protocol_addr(t, addr);
	}
	return 0;
}

enum transport_type transport_type(struct transport *t)
{
	return t->type;
}

struct transport *transport_create(struct config *cfg,
				   enum transport_type type)
{
	struct transport *t = NULL;
	switch (type) {
	case TRANS_UDS:
		t = uds_transport_create();
		break;
	case TRANS_UDP_IPV4:
		t = udp_transport_create();
		break;
	case TRANS_UDP_IPV6:
		t = udp6_transport_create();
		break;
	case TRANS_IEEE_802_3:
		t = raw_transport_create();
		break;
	case TRANS_DEVICENET:
	case TRANS_CONTROLNET:
	case TRANS_PROFINET:
		break;
	}
	if (t) {
		t->type = type;
		t->cfg = cfg;
	}
	return t;
}

void transport_destroy(struct transport *t)
{
	t->release(t);
}
