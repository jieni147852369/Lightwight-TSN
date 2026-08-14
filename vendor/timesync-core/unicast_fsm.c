#include "unicast_fsm.h"

enum unicast_state unicast_fsm(enum unicast_state state, enum unicast_event ev)
{
	enum unicast_state next = state;

	switch (state) {
	case UC_WAIT:
		switch (ev) {
		case UC_EV_GRANT_ANN:
			next = UC_HAVE_ANN;
			break;
		case UC_EV_SELECTED:
		case UC_EV_GRANT_SYDY:
		case UC_EV_UNSELECTED:
		case UC_EV_CANCEL:
			break;
		}
		break;
	case UC_HAVE_ANN:
		switch (ev) {
		case UC_EV_GRANT_ANN:
			break;
		case UC_EV_SELECTED:
			next = UC_NEED_SYDY;
			break;
		case UC_EV_GRANT_SYDY:
		case UC_EV_UNSELECTED:
			break;
		case UC_EV_CANCEL:
			next = UC_WAIT;
			break;
		}
		break;
	case UC_NEED_SYDY:
		switch (ev) {
		case UC_EV_GRANT_ANN:
		case UC_EV_SELECTED:
			break;
		case UC_EV_GRANT_SYDY:
			next = UC_HAVE_SYDY;
			break;
		case UC_EV_UNSELECTED:
			next = UC_HAVE_ANN;
			break;
		case UC_EV_CANCEL:
			next = UC_WAIT;
			break;
		}
		break;
	case UC_HAVE_SYDY:
		switch (ev) {
		case UC_EV_GRANT_ANN:
		case UC_EV_SELECTED:
		case UC_EV_GRANT_SYDY:
			break;
		case UC_EV_UNSELECTED:
			next = UC_HAVE_ANN;
			break;
		case UC_EV_CANCEL:
			next = UC_WAIT;
			break;
		}
		break;
	}
	return next;
}
