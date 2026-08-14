#ifndef HAVE_FSM_H
#define HAVE_FSM_H

/** Defines the state of a port. */
enum port_state {
	PS_INITIALIZING = 1,
	PS_FAULTY,
	PS_DISABLED,
	PS_LISTENING,
	PS_PRE_MASTER,
	PS_MASTER,
	PS_PASSIVE,
	PS_UNCALIBRATED,
	PS_SLAVE,
	PS_GRAND_MASTER, /*non-standard extension*/
};

/** Defines the events for the port state machine. */
enum fsm_event {
	EV_NONE,
	EV_POWERUP,
	EV_INITIALIZE,
	EV_DESIGNATED_ENABLED,
	EV_DESIGNATED_DISABLED,
	EV_FAULT_CLEARED,
	EV_FAULT_DETECTED,
	EV_STATE_DECISION_EVENT,
	EV_QUALIFICATION_TIMEOUT_EXPIRES,
	EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES,
	EV_SYNCHRONIZATION_FAULT,
	EV_MASTER_CLOCK_SELECTED,
	EV_INIT_COMPLETE,
	EV_RS_MASTER,
	EV_RS_GRAND_MASTER,
	EV_RS_SLAVE,
	EV_RS_PASSIVE,
};

enum bmca_select {
	BMCA_PTP,
	BMCA_NOOP,
};

/**
 * Run the state machine for a BC or OC port.
 * @param state  The current state of the port.
 * @param event  The event to be processed.
 * @param mdiff  Whether a new master has been selected.
 * @return       The new state for the port.
 */
enum port_state ptp_fsm(enum port_state state, enum fsm_event event, int mdiff);

/**
 * Run the state machine for a slave only clock.
 * @param state  The current state of the port.
 * @param event  The event to be processed.
 * @param mdiff  Whether a new master has been selected.
 * @return       The new state for the port.
 */
enum port_state ptp_slave_fsm(enum port_state state, enum fsm_event event,
			      int mdiff);

#endif
