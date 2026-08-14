#ifndef HAVE_DESIGNATED_FSM_H
#define HAVE_DESIGNATED_FSM_H

#include "fsm.h"

/**
 * Run the state machine for a clock which is designated as master port.
 * @param state  The current state of the port.
 * @param event  The event to be processed.
 * @param mdiff  This param is not used by this function.
 * @return       The new state for the port.
 */
enum port_state designated_master_fsm(enum port_state state,
				      enum fsm_event event,
				      int mdiff);

/**
 * Run the state machine for a clock designated as slave port.
 * @param state  The current state of the port.
 * @param event  The event to be processed.
 * @param mdiff  This param is not used by this function.
 * @return       The new state for the port.
 */
enum port_state designated_slave_fsm(enum port_state state,
				     enum fsm_event event,
				     int mdiff);
#endif
