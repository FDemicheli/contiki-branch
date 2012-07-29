/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Common functionality for phase optimization in duty cycling radio protocols
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/* Modified by RMonica
 * Patches: - different nodes may have different cycle times
 *          - add RPL function RPL_DAG_MC_AVG_DELAY
 */

#ifndef PHASE_H
#define PHASE_H

#include "net/rime/rimeaddr.h"
#include "sys/timer.h"
#include "sys/rtimer.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/netstack.h"

#if PHASE_CONF_DRIFT_CORRECT
#define PHASE_DRIFT_CORRECT PHASE_CONF_DRIFT_CORRECT
#else
#define PHASE_DRIFT_CORRECT 0
#endif

struct phase {
  struct phase *next;
  rimeaddr_t neighbor;
  rtimer_clock_t time;
  rtimer_cycle_time_t cycle_time; // cycle time of the node (may differ from that of this node)
#if PHASE_DRIFT_CORRECT
  rtimer_clock_t drift;
#endif
  uint8_t noacks;
  struct timer noacks_timer;
};

struct phase_list {
  list_t *list;
  struct memb *memb;
};

typedef enum {
  PHASE_UNKNOWN,
  PHASE_SEND_NOW,
  PHASE_DEFERRED,
} phase_status_t;


#define PHASE_LIST(name, num) LIST(phase_list_list);                              \
                              MEMB(phase_list_memb, struct phase, num);           \
                              struct phase_list name = { &phase_list_list, &phase_list_memb }

void phase_init(struct phase_list *list);
phase_status_t phase_wait(struct phase_list *list,  const rimeaddr_t *neighbor,
                          rtimer_clock_t wait_before,
                          mac_callback_t mac_callback, void *mac_callback_ptr,
                          struct rdc_buf_list *buf_list);
void phase_update(const struct phase_list *list, const rimeaddr_t *neighbor,
                  rtimer_clock_t time, int mac_status);

void phase_remove(const struct phase_list *list, const rimeaddr_t *neighbor);

/* Function added by RMonica
 * see implementation (phase.c)
 */
rtimer_cycle_time_t phase_get_average_delay(const struct phase_list *list, const rimeaddr_t *neighbor,
                                       rtimer_clock_t guard_time, rtimer_clock_t my_phase);

/* Function added by RMonica
 * see implementation (phase.c)
 */
void cycle_time_update(const struct phase_list *list,
             const rimeaddr_t *neighbor, rtimer_cycle_time_t cycle_time);

#endif /* PHASE_H */
