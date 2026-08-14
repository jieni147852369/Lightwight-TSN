#ifndef HAVE_TC_H
#define HAVE_TC_H

#include "msg.h"
#include "port_private.h"

/**
 * Flushes the list of remembered residence times.
 * @param q    Port whose list should be flushed
 */
void tc_flush(struct port *q);

/**
 * Forwards a given general message out all other ports.
 * @param q    The ingress port
 * @param msg  The message to be sent
 * @return     Zero on success, non-zero otherwise.
 */
int tc_forward(struct port *q, struct ptp_message *msg);

/**
 * Forwards a given Follow-Up message out all other ports.
 *
 * This function adds the unique, per egress port residence time into
 * the correction field for the transmitted follow up message.
 *
 * @param q    The ingress port
 * @param msg  The message to be sent
 * @return     Zero on success, non-zero otherwise.
 */
int tc_fwd_folup(struct port *q, struct ptp_message *msg);

/**
 * Forwards a given delay request message out all other ports.
 *
 * This function computes the unique residence time for each egress
 * port, remembering it in that egress port.
 *
 * @param q    The ingress port
 * @param msg  The message to be sent
 * @return     Zero on success, non-zero otherwise.
 */
int tc_fwd_request(struct port *q, struct ptp_message *msg);

/**
 * Forwards a given response message out all other ports.
 *
 * This function adds the unique, per egress port residence time into
 * the correction field for the transmitted delay response message.
 *
 * @param q    The ingress port
 * @param msg  The message to be sent
 * @return     Zero on success, non-zero otherwise.
 */
int tc_fwd_response(struct port *q, struct ptp_message *msg);

/**
 * Forwards a given sync message out all other ports.
 *
 * This function computes the unique residence time for each egress
 * port, remembering it in that egress port.
 *
 * @param q    The ingress port
 * @param msg  The message to be sent
 * @return     Zero on success, non-zero otherwise.
 */
int tc_fwd_sync(struct port *q, struct ptp_message *msg);

/**
 * Determines whether the local clock should ignore a given message.
 *
 * @param q    The ingress port
 * @param msg  The message to test
 * @return     One if the message should be ignored, zero otherwise.
 */
int tc_ignore(struct port *q, struct ptp_message *m);

/**
 * Prunes stale entries from the list of remembered residence times.
 * @param q    Port whose list should be pruned.
 */
void tc_prune(struct port *q);

#endif
