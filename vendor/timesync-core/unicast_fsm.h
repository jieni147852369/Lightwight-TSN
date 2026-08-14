#ifndef HAVE_UNICAST_FSM_H
#define HAVE_UNICAST_FSM_H

enum unicast_state {
	UC_WAIT,
	UC_HAVE_ANN,
	UC_NEED_SYDY,
	UC_HAVE_SYDY,
};

enum unicast_event {
	UC_EV_GRANT_ANN,
	UC_EV_SELECTED,
	UC_EV_GRANT_SYDY,
	UC_EV_UNSELECTED,
	UC_EV_CANCEL,
};

enum unicast_state unicast_fsm(enum unicast_state state, enum unicast_event ev);

#endif
