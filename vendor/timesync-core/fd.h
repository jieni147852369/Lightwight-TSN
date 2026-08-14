#ifndef HAVE_FD_H
#define HAVE_FD_H

#define N_TIMER_FDS 8

/*
 * The order matters here.  The DELAY timer must appear before the
 * ANNOUNCE and SYNC_RX timers in order to correctly handle the case
 * when the DELAY timer and one of the other two expire during the
 * same call to poll().
 */
enum {
	FD_EVENT,
	FD_GENERAL,
	FD_DELAY_TIMER,
	FD_ANNOUNCE_TIMER,
	FD_SYNC_RX_TIMER,
	FD_QUALIFICATION_TIMER,
	FD_MANNO_TIMER,
	FD_SYNC_TX_TIMER,
	FD_UNICAST_REQ_TIMER,
	FD_UNICAST_SRV_TIMER,
	FD_RTNL,
	N_POLLFD,
};

#define FD_FIRST_TIMER FD_DELAY_TIMER

struct fdarray {
	int fd[N_POLLFD];
};

#endif
