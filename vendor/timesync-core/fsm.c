#include "fsm.h"

enum port_state ptp_fsm(enum port_state state, enum fsm_event event, int mdiff)
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
			next = PS_LISTENING;
			break;
		default:
			break;
		}
		break;

	case PS_FAULTY:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_CLEARED:
			next = PS_INITIALIZING;
			break;
		default:
			break;
		}
		break;

	case PS_DISABLED:
		if (EV_DESIGNATED_ENABLED == event)
			next = PS_INITIALIZING;
		break;

	case PS_LISTENING:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			next = PS_MASTER;
			break;
		case EV_RS_MASTER:
			next = PS_PRE_MASTER;
			break;
		case EV_RS_GRAND_MASTER:
			next = PS_GRAND_MASTER;
			break;
		case EV_RS_SLAVE:
			next = PS_UNCALIBRATED;
			break;
		case EV_RS_PASSIVE:
			next = PS_PASSIVE;
			break;
		default:
			break;
		}
		break;

	case PS_PRE_MASTER:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_QUALIFICATION_TIMEOUT_EXPIRES:
			next = PS_MASTER;
			break;
		case EV_RS_SLAVE:
			next = PS_UNCALIBRATED;
			break;
		case EV_RS_PASSIVE:
			next = PS_PASSIVE;
			break;
		default:
			break;
		}
		break;

	case PS_MASTER:
	case PS_GRAND_MASTER:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_RS_SLAVE:
			next = PS_UNCALIBRATED;
			break;
		case EV_RS_PASSIVE:
			next = PS_PASSIVE;
			break;
		default:
			break;
		}
		break;

	case PS_PASSIVE:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			next = PS_MASTER;
			break;
		case EV_RS_MASTER:
			next = PS_PRE_MASTER;
			break;
		case EV_RS_GRAND_MASTER:
			next = PS_GRAND_MASTER;
			break;
		case EV_RS_SLAVE:
			next = PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;

	case PS_UNCALIBRATED:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			next = PS_MASTER;
			break;
		case EV_MASTER_CLOCK_SELECTED:
			next = PS_SLAVE;
			break;
		case EV_RS_MASTER:
			next = PS_PRE_MASTER;
			break;
		case EV_RS_GRAND_MASTER:
			next = PS_GRAND_MASTER;
			break;
		case EV_RS_SLAVE:
			next = PS_UNCALIBRATED;
			break;
		case EV_RS_PASSIVE:
			next = PS_PASSIVE;
			break;
		default:
			break;
		}
		break;

	case PS_SLAVE:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			next = PS_MASTER;
			break;
		case EV_SYNCHRONIZATION_FAULT:
			next = PS_UNCALIBRATED;
			break;
		case EV_RS_MASTER:
			next = PS_PRE_MASTER;
			break;
		case EV_RS_GRAND_MASTER:
			next = PS_GRAND_MASTER;
			break;
		case EV_RS_SLAVE:
			if (mdiff)
				next = PS_UNCALIBRATED;
			break;
		case EV_RS_PASSIVE:
			next = PS_PASSIVE;
			break;
		default:
			break;
		}
		break;
	}

	return next;
}

enum port_state ptp_slave_fsm(enum port_state state, enum fsm_event event,
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
			next = PS_LISTENING;
			break;
		default:
			break;
		}
		break;

	case PS_FAULTY:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_CLEARED:
			next = PS_INITIALIZING;
			break;
		default:
			break;
		}
		break;

	case PS_DISABLED:
		if (EV_DESIGNATED_ENABLED == event)
			next = PS_INITIALIZING;
		break;

	case PS_LISTENING:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
		case EV_RS_MASTER:
		case EV_RS_GRAND_MASTER:
		case EV_RS_PASSIVE:
			next = PS_LISTENING;
			break;
		case EV_RS_SLAVE:
			next = PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;

	case PS_UNCALIBRATED:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
		case EV_RS_MASTER:
		case EV_RS_GRAND_MASTER:
		case EV_RS_PASSIVE:
			next = PS_LISTENING;
			break;
		case EV_MASTER_CLOCK_SELECTED:
			next = PS_SLAVE;
			break;
		default:
			break;
		}
		break;

	case PS_SLAVE:
		switch (event) {
		case EV_DESIGNATED_DISABLED:
			next = PS_DISABLED;
			break;
		case EV_FAULT_DETECTED:
			next = PS_FAULTY;
			break;
		case EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
		case EV_RS_MASTER:
		case EV_RS_GRAND_MASTER:
		case EV_RS_PASSIVE:
			next = PS_LISTENING;
			break;
		case EV_SYNCHRONIZATION_FAULT:
			next = PS_UNCALIBRATED;
			break;
		case EV_RS_SLAVE:
			if (mdiff)
				next = PS_UNCALIBRATED;
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
