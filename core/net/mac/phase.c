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
 *          - phase discovery by test packet
 */

#include "net/mac/phase.h"
#include "net/packetbuf.h"
#include "sys/clock.h"
#include "lib/memb.h"
#include "sys/ctimer.h"
#include "net/queuebuf.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "net/neighbor-info.h"

struct phase_queueitem {
  struct ctimer timer;
  mac_callback_t mac_callback;
  void *mac_callback_ptr;
  struct queuebuf *q;
  struct rdc_buf_list *buf_list;
};

#define PHASE_DEFER_THRESHOLD 1
#define PHASE_QUEUESIZE       8

#define MAX_NOACKS            16

#define MAX_NOACKS_TIME       CLOCK_SECOND * 30

#define UNKNOWN_CYCLE_TIME (MAX_CYCLE_TIME + 1)     // no cycle time may be higher than MAX_CYCLE_TIME
#define UNKNOWN_TIME       (RTIMER_ARCH_SECOND + 1) // phases are saved mod 1 second

/* When using the routing function RPL_DAG_MC_AVG_DELAY
 * the following define may be set to 1
 * to send test packets and discover the phase of the neighbors
 * and make faster decisions during the formation of the DAG
 */
#ifndef PHASE_DISCOVERY_USE_TEST_PACKET
#define PHASE_DISCOVERY_USE_TEST_PACKET 1
#endif
#define UNKNOWN_TIME_WAITING_FOR_PHASE (RTIMER_ARCH_SECOND + 2)   // phase discovery in progress
#define UNKNOWN_TIME_PHASE_DISC_FAILED (RTIMER_ARCH_SECOND + 3)   // phase discovery failed

MEMB(queued_packets_memb, struct phase_queueitem, PHASE_QUEUESIZE);

#define DEBUG 0
//#define DEBUG 1 ///to print
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTDEBUG(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#define PRINTDEBUG(...)
#endif
/*---------------------------------------------------------------------------*/
struct phase *
find_neighbor(const struct phase_list *list, const rimeaddr_t *addr)
{
  struct phase *e;
  for(e = list_head(*list->list); e != NULL; e = list_item_next(e)) {
    if(rimeaddr_cmp(addr, &e->neighbor)) {
      return e;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Function added by RMonica
 *
 * this function initializes a variable of type "struct phase" (defined in phase.h)
 * with default values
 */
void
init_single_phase(struct phase * e)
{
  e->time = UNKNOWN_TIME;
#if PHASE_DRIFT_CORRECT
  e->drift = 0;
#endif
  e->noacks = 0;
  e->cycle_time = UNKNOWN_CYCLE_TIME;
}
/*---------------------------------------------------------------------------*/
void
phase_remove(const struct phase_list *list, const rimeaddr_t *neighbor)
{
  struct phase *e;
  e = find_neighbor(list, neighbor);
  if(e != NULL) {
    list_remove(*list->list, e);
    memb_free(list->memb, e);
  }
}
/*---------------------------------------------------------------------------*/
void
phase_update(const struct phase_list *list,
             const rimeaddr_t *neighbor, rtimer_clock_t time,
             int mac_status)
{
  struct phase *e;

// Modification by RMonica
// avoid saving phases higher than one second (all cycle times are divisors of the second)
// because number greater than RTIMER_ARCH_SECOND are special values (UNKNOWN_TIME and so on)
#if RTIMER_ARCH_SECOND & (RTIMER_ARCH_SECOND - 1)
  time %= RTIMER_ARCH_SECOND;
#else
  time &= RTIMER_ARCH_SECOND - 1;
#endif

  /* If we have an entry for this neighbor already, we renew it. */
  e = find_neighbor(list, neighbor);
  if(e != NULL) {
    if(mac_status == MAC_TX_OK) {
#if PHASE_DRIFT_CORRECT
      e->drift = time-e->time;
#endif
      e->time = time;
    }
    /* If the neighbor didn't reply to us, it may have switched
       phase (rebooted). We try a number of transmissions to it
       before we drop it from the phase list. */
    if(mac_status == MAC_TX_NOACK) {
      PRINTF("phase noacks %d to %d.%d\n", e->noacks, neighbor->u8[0], neighbor->u8[1]);
      e->noacks++;
      if(e->noacks == 1) {
        timer_set(&e->noacks_timer, MAX_NOACKS_TIME);
      }
      if(e->noacks >= MAX_NOACKS || timer_expired(&e->noacks_timer)) {
        PRINTF("drop %d\n", neighbor->u8[0]);
        list_remove(*list->list, e);
        memb_free(list->memb, e);
        return;
      }
    } else if(mac_status == MAC_TX_OK) {
      e->noacks = 0;
    }
  } else {
    /* No matching phase was found, so we allocate a new one. */
    if(mac_status == MAC_TX_OK && e == NULL) {
      e = memb_alloc(list->memb);
      if(e == NULL) {
        PRINTF("phase alloc NULL\n");
        /* We could not allocate memory for this phase, so we drop
           the last item on the list and reuse it for our phase. */
        e = list_chop(*list->list);
      }
      rimeaddr_copy(&e->neighbor, neighbor);
      init_single_phase(e);
      e->time = time;
      list_push(*list->list, e);
    }
  }

  // Modification by RMonica
  neighbor_info_other_source_metric_update(neighbor, 1); // notify change to RPL
}
/*---------------------------------------------------------------------------*/
/* Function added by RMonica
 * this function works similarly to phase_update
 * but stores the cycle time of a neighbor node and not its phase
 */
void
cycle_time_update(const struct phase_list *list,
             const rimeaddr_t *neighbor, rtimer_cycle_time_t cycle_time)
{
  struct phase *e;

  /* If we have an entry for this neighbor already, we renew it. */
  e = find_neighbor(list, neighbor);
  if(e != NULL) {
    if (e->cycle_time != cycle_time) {
      e->cycle_time = cycle_time;
      neighbor_info_other_source_metric_update(neighbor, 1); // notify change to RPL
    }
  }
  else {
    e = memb_alloc(list->memb);
    if(e == NULL) {
      PRINTF("phase alloc NULL\n");
        /* We could not allocate memory for this phase, so we drop
           the last item on the list and reuse it for our phase. */
      e = list_chop(*list->list);
    }
    rimeaddr_copy(&e->neighbor, neighbor);
    init_single_phase(e);
    e->cycle_time = cycle_time; // we only know the cycle time
    list_push(*list->list, e);
    neighbor_info_other_source_metric_update(neighbor, 1); // notify change to RPL
  }
}
/*---------------------------------------------------------------------------*/
#if PHASE_DISCOVERY_USE_TEST_PACKET
/* Variables added by RMonica
 *
 * While a phase discovery packet has been sent to the MAC layer, and we're waiting for a response
 * we must remember: the destination address of the packet and the phase list we're trying to update
 * so, when the response arrives, we update the recorded list at the recorded address.
 *
 * Since it's very unlikely that more than one discovery is in progress (because DIOs are received one by one)
 * we allocate only the space for one discovery
 * if multiple simultaneous discoveries are requested, we'll reject them all but one
 */
static rimeaddr_t phase_discovery_dest;
static const struct phase_list * phase_discovery_list = NULL; // if NULL, no phase discovery is in progress
/* Function added by RMonica
 * This callback is called by the MAC layer when the transmission of the phase discovery packet
 * ended (succeeded or failed)
 */
static void
phase_discovery_callback(void * ptr, int status, int num_transmissions)
{
  if (status != MAC_TX_OK) {
    struct phase * e = find_neighbor(phase_discovery_list, &phase_discovery_dest);
    if (e && e->time >= RTIMER_ARCH_SECOND) {
      //printf("phase discovery failed.\n");
      e->time = UNKNOWN_TIME_PHASE_DISC_FAILED;
    }
  }

  phase_discovery_list = NULL; // mark that a phase discovery is no longer in progress

  neighbor_info_packet_sent(status, num_transmissions); // notify change to RPL
}
/* Function added by RMonica
 * Sends an useless packet to the neighbor with mac address "towho"
 * to discover its phase and save it in the phase list "list".
 * Before calling this, you need to check if a previous phase discovery is in progress
 * or the previous discovery could be overridden (see phase_discovery_list)
 */
void
phase_send_phase_discovery(const rimeaddr_t * towho, const struct phase_list *list)
{
  packetbuf_clear();
  packetbuf_set_datalen(sizeof(uint8_t));
  packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE,
                     PACKETBUF_ATTR_PACKET_TYPE_DATA);
  // this packet will be rejected by FRAMER.parse() of the receiver
  // but the only thing that matters is that the sender receives the ACK
  *((uint8_t *)(packetbuf_dataptr())) = 1; // just put a byte of data in it

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &rimeaddr_node_addr);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, towho);

  // store data for the callback
  rimeaddr_copy(&phase_discovery_dest, towho);
  phase_discovery_list = list;

  NETSTACK_MAC.send(phase_discovery_callback, NULL);
}
#endif /* PHASE_DISCOVERY_USE_TEST_PACKET */
/*---------------------------------------------------------------------------*/
/* Function added by RMonica
 * this function gets the average delay for a neighbor node, for the routing function
 * RPL_DAG_MC_AVG_DELAY.
 *
 * It requires:
 * - the list of the phases of the neighbors
 * - the MAC address of the neighbor of which the delay must be calculated
 * - the minimum relay time, here called guard_time
 * - the duty cycle phase of the current node
 */
rtimer_cycle_time_t phase_get_average_delay(const struct phase_list *list, const rimeaddr_t *neighbor,
                                       rtimer_clock_t guard_time, rtimer_clock_t my_phase)
{
  struct phase * e = find_neighbor(list,neighbor);
  if (e && e->cycle_time != UNKNOWN_CYCLE_TIME && e->cycle_time != 0) { // cycle time exists and is known

    if (e->cycle_time == CYCLE_TIME) { // same cycle time
      if (e->time < RTIMER_ARCH_SECOND) { // known phase
#if (CYCLE_TIME & (CYCLE_TIME >> 1))   // works in general
        rtimer_cycle_time_t result = ((rtimer_clock_t)(e->time - my_phase)) % CYCLE_TIME;
#else   // works only if CYCLE_TIME is a power of two
        rtimer_cycle_time_t result = ((rtimer_clock_t)(e->time - my_phase)) & (CYCLE_TIME - 1);
#endif

        if (result < guard_time) {
          result += CYCLE_TIME; // phase too near, we have to send during next cycle
        }

        return result;          // known phase and cycle time
      }

#if PHASE_DISCOVERY_USE_TEST_PACKET
      if (e->time == UNKNOWN_TIME_PHASE_DISC_FAILED) { // discovery failed: use RPL to discover
        return guard_time;
        }

      if (e->time != UNKNOWN_TIME_WAITING_FOR_PHASE) { // unknown phase and not waiting for the phase discovery
        if (!phase_discovery_list) {                   // if no other phase discovery is in progress
          e->time = UNKNOWN_TIME_WAITING_FOR_PHASE;
          phase_send_phase_discovery(neighbor, list);  // discover the phase
        }
      }

      return e->cycle_time + guard_time; // same cycle time, but unknown phase: pessimism
#else
      return guard_time; // use RPL to discover
#endif
    }
    return (e->cycle_time >> 1) + guard_time; // different cycle times: average delay: cycle_time / 2
  }
  return guard_time; // cycle time is unknown or 0
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  struct phase_queueitem *p = ptr;

  if(p->buf_list == NULL) {
    queuebuf_to_packetbuf(p->q);
    queuebuf_free(p->q);
    NETSTACK_RDC.send(p->mac_callback, p->mac_callback_ptr);
  } else {
    NETSTACK_RDC.send_list(p->mac_callback, p->mac_callback_ptr, p->buf_list);
  }

  memb_free(&queued_packets_memb, p);
}
/*---------------------------------------------------------------------------*/
/* Function modified by RMonica
 * it now uses the specific cycle time of the receiver instead of CYCLE_TIME
 * defined in configuration, so different nodes may use different cycle times
 * the parameter cycle_time has been removed, because cycle times are stored in the phase list
 */
phase_status_t
phase_wait(struct phase_list *list,
           const rimeaddr_t *neighbor,
           rtimer_clock_t guard_time,
           mac_callback_t mac_callback, void *mac_callback_ptr,
           struct rdc_buf_list *buf_list)
{
  struct phase *e;
  //  const rimeaddr_t *neighbor = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  /* We go through the list of phases to find if we have recorded a
     phase for this particular neighbor. If so, we can compute the
     time for the next expected phase and setup a ctimer to switch on
     the radio just before the phase. */
  e = find_neighbor(list, neighbor);
  if(e != NULL && !e->cycle_time) {
    return PHASE_SEND_NOW;  // the node is always on
  }

  if(e != NULL && (e->cycle_time != UNKNOWN_CYCLE_TIME) && (e->time < RTIMER_ARCH_SECOND)) {
    rtimer_clock_t wait, now, expected, sync;
    clock_time_t ctimewait;
    
    /* We expect phases to happen every CYCLE_TIME time
       units. The next expected phase is at time e->time +
       CYCLE_TIME. To compute a relative offset, we subtract
       with clock_time(). Because we are only interested in turning
       on the radio within the CYCLE_TIME period, we compute the
       waiting time with modulo CYCLE_TIME. */
    
    /*      printf("neighbor phase 0x%02x (cycle 0x%02x)\n", e->time & (cycle_time - 1),
            cycle_time);*/

    /*      if(e->noacks > 0) {
            printf("additional wait %d\n", additional_wait);
            }*/
    
    now = RTIMER_NOW();

    sync = e->time;

#if PHASE_DRIFT_CORRECT
    {
      int32_t s;
      if(e->drift > e->cycle_time) {
        s = e->drift % e->cycle_time / (e->drift / e->cycle_time);  /* drift per cycle */
        s = s * (now - sync) / e->cycle_time;                       /* estimated drift to now */
        sync += s;                                                  /* add it in */
      }
    }
#endif

    /* Check if cycle_time is a power of two */
    if(!(e->cycle_time & (e->cycle_time - 1))) {
      /* Faster if cycle_time is a power of two */
      wait = (rtimer_clock_t)((sync - now) & (e->cycle_time - 1));
    } else {
      /* Works generally */
      wait = e->cycle_time - ((rtimer_clock_t)(now - sync) % e->cycle_time);
    }

    if(wait < guard_time) {
      wait += e->cycle_time;
    }

    ctimewait = (CLOCK_SECOND * (wait - guard_time)) / RTIMER_ARCH_SECOND;

    if(ctimewait > PHASE_DEFER_THRESHOLD) {
      struct phase_queueitem *p;
      
      p = memb_alloc(&queued_packets_memb);
      if(p != NULL) {
        if(buf_list == NULL) {
          p->q = queuebuf_new_from_packetbuf();
        }
        p->mac_callback = mac_callback;
        p->mac_callback_ptr = mac_callback_ptr;
        p->buf_list = buf_list;
        ctimer_set(&p->timer, ctimewait, send_packet, p);
        return PHASE_DEFERRED;
      } else {
        memb_free(&queued_packets_memb, p);
      }
    }

    expected = now + wait - guard_time;
    if(!RTIMER_CLOCK_LT(expected, now)) {
      /* Wait until the receiver is expected to be awake */
//    printf("%d ",expected%cycle_time);  //for spreadsheet export
      while(RTIMER_CLOCK_LT(RTIMER_NOW(), expected));
    }
    return PHASE_SEND_NOW;
  }
  return PHASE_UNKNOWN;
}
/*---------------------------------------------------------------------------*/
void
phase_init(struct phase_list *list)
{
  list_init(*list->list);
  memb_init(list->memb);
  memb_init(&queued_packets_memb);
}
/*---------------------------------------------------------------------------*/
