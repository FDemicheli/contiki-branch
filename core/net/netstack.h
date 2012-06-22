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
 * $Id: netstack.h,v 1.6 2010/10/03 20:37:32 adamdunkels Exp $
 */

/**
 * \file
 *         Include file for the Contiki low-layer network stack (NETSTACK)
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef NETSTACK_H
#define NETSTACK_H

#include "contiki-conf.h"

#ifndef NETSTACK_NETWORK
#ifdef NETSTACK_CONF_NETWORK
#define NETSTACK_NETWORK NETSTACK_CONF_NETWORK
#else /* NETSTACK_CONF_NETWORK */
#define NETSTACK_NETWORK rime_driver
#endif /* NETSTACK_CONF_NETWORK */
#endif /* NETSTACK_NETWORK */

#ifndef NETSTACK_MAC
#ifdef NETSTACK_CONF_MAC
#define NETSTACK_MAC NETSTACK_CONF_MAC
#else /* NETSTACK_CONF_MAC */
#define NETSTACK_MAC     nullmac_driver
#endif /* NETSTACK_CONF_MAC */
#endif /* NETSTACK_MAC */

#ifndef NETSTACK_RDC
#ifdef NETSTACK_CONF_RDC
#define NETSTACK_RDC NETSTACK_CONF_RDC
#else /* NETSTACK_CONF_RDC */
#define NETSTACK_RDC     nullrdc_driver
#endif /* NETSTACK_CONF_RDC */
#endif /* NETSTACK_RDC */

#ifndef NETSTACK_RDC_CHANNEL_CHECK_RATE
#ifdef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_RDC_CHANNEL_CHECK_RATE NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#else /* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE */
#define NETSTACK_RDC_CHANNEL_CHECK_RATE 8
#endif /* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE */
#endif /* NETSTACK_RDC_CHANNEL_CHECK_RATE */

/* CYCLE_TIME for channel cca checks, in rtimer ticks. */
#ifdef CONTIKIMAC_CONF_CYCLE_RATE
#define CYCLE_TIME (RTIMER_ARCH_SECOND / CONTIKIMAC_CONF_CYCLE_RATE)
#define CYCLE_RATE (CONTIKIMAC_CONF_CYCLE_RATE)
#else
#define CYCLE_TIME (RTIMER_ARCH_SECOND / NETSTACK_RDC_CHANNEL_CHECK_RATE)
#define CYCLE_RATE (NETSTACK_RDC_CHANNEL_CHECK_RATE)
#endif

// if the cycle rate of a node is higher than this, phase optimization to that node will be disabled
#define APPROX_RADIO_ALWAYS_ON_CYCLE_RATE 64
#define APPROX_RADIO_ALWAYS_ON_CYCLE_TIME (RTIMER_ARCH_SECOND / APPROX_RADIO_ALWAYS_ON_CYCLE_RATE)

// max cycle time for the whole network (in case different duty cycles)
#ifdef CONTIKIMAC_CONF_MIN_CYCLE_RATE
#define MAX_CYCLE_TIME (RTIMER_ARCH_SECOND / CONTIKIMAC_CONF_MIN_CYCLE_RATE)
#define MIN_CYCLE_RATE (CONTIKIMAC_CONF_MIN_CYCLE_RATE)
#else
#define MAX_CYCLE_TIME (CYCLE_TIME)
#define MIN_CYCLE_RATE (CYCLE_RATE)
#endif

#if MAX_CYCLE_TIME <= 65535  // save memory
typedef uint16_t rtimer_cycle_time_t;
#else
typedef uint32_t rtimer_cycle_time_t;
#endif

#include "net/rime/rimeaddr.h"
void contikimac_cycle_time_update(const rimeaddr_t *addr,rtimer_cycle_time_t newtime);
rtimer_cycle_time_t contikimac_get_cycle_time_for_routing();

#if (NETSTACK_RDC_CHANNEL_CHECK_RATE & (NETSTACK_RDC_CHANNEL_CHECK_RATE - 1)) != 0
#error NETSTACK_RDC_CONF_CHANNEL_CHECK_RATE must be a power of two (i.e., 1, 2, 4, 8, 16, 32, 64, ...).
#error Change NETSTACK_RDC_CONF_CHANNEL_CHECK_RATE in contiki-conf.h, project-conf.h or in your Makefile.
#endif


#ifndef NETSTACK_RADIO
#ifdef NETSTACK_CONF_RADIO
#define NETSTACK_RADIO NETSTACK_CONF_RADIO
#else /* NETSTACK_CONF_RADIO */
#define NETSTACK_RADIO   nullradio_driver
#endif /* NETSTACK_CONF_RADIO */
#endif /* NETSTACK_RADIO */

#ifndef NETSTACK_FRAMER
#ifdef NETSTACK_CONF_FRAMER
#define NETSTACK_FRAMER NETSTACK_CONF_FRAMER
#else /* NETSTACK_CONF_FRAMER */
#define NETSTACK_FRAMER   framer_nullmac
#endif /* NETSTACK_CONF_FRAMER */
#endif /* NETSTACK_FRAMER */

#include "net/mac/mac.h"
#include "net/mac/rdc.h"
#include "net/mac/framer.h"
#include "dev/radio.h"

/**
 * The structure of a network driver in Contiki.
 */
struct network_driver {
  char *name;

  /** Initialize the network driver */
  void (* init)(void);

  /** Callback for getting notified of incoming packet. */
  void (* input)(void);
};

extern const struct network_driver NETSTACK_NETWORK;
extern const struct rdc_driver     NETSTACK_RDC;
extern const struct mac_driver     NETSTACK_MAC;
extern const struct radio_driver   NETSTACK_RADIO;
extern const struct framer         NETSTACK_FRAMER;

void netstack_init(void);

#endif /* NETSTACK_H */
