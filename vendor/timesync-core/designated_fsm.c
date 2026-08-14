#include "fsm.h"
#include "designated_fsm.h"

enum port_state designated_master_fsm(enum port_state state,
				      enum fsm_event event,
				      int mdiff)
{
	enum port_state next = state;

	if (EV_INITIALIZE == event || EV_POWERUP == event)
		return PS_INITIALIZING;

	switch (state) {
	case PS_INITIALIZING:
		switch (event) {
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_INIT_COMPLETE:
			next = PS_MASTER;
			break;
		default:
			break;
		}
		break;

	case PS_FAULTY:
		if (event == EV_FAULT_CLEARED) {
			next = PS_INITIALIZING;
		}
		break;

	case PS_MASTER:
		if (event == EV_FAULT_DETECTED) {
			next = PS_FAULTY;
		}
		break;

	default:
		break;
	}
	return next;
}

enum port_state designated_slave_fsm(enum port_state state,
				     enum fsm_event event,
				     int mdiff)
{
	enum port_state next = state;

	if (EV_INITIALIZE == event || EV_POWERUP == event)
		return PS_INITIALIZING;

	switch (state) {
	case PS_INITIALIZING:
		switch (event) {
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_INIT_COMPLETE:
			next =  PS_SLAVE;
			break;
		default:
			break;
		}
		break;

	case PS_FAULTY:
		if (event == EV_FAULT_CLEARED) {
			next = PS_INITIALIZING;
		}
		break;

	case PS_SLAVE:
		switch (event) {
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
	return next;
}
