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
 * $Id: neighbor-info.c,v 1.18 2010/12/15 14:35:07 nvt-se Exp $
 */
/**
 * \file
 *         A generic module for management of neighbor information.
 *
 * \author Nicolas Tsiftes <nvt@sics.se>
 */

#include "net/neighbor-info.h"
#include "net/neighbor-attr.h"
#include "net/uip-ds6.h"
#include "net/uip-nd6.h"

#define DEBUG DEBUG_NONE
//#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#define ETX_LIMIT		15
#define ETX_SCALE		100
#define ETX_ALPHA		90
#define ETX_NOACK_PENALTY       ETX_LIMIT
/*---------------------------------------------------------------------------*/
NEIGHBOR_ATTRIBUTE_GLOBAL(link_metric_t, attr_etx, NULL);
NEIGHBOR_ATTRIBUTE_GLOBAL(unsigned long, attr_timestamp, NULL);

static neighbor_info_subscriber_t subscriber_callback;
/*---------------------------------------------------------------------------*/
static void
update_metric(const rimeaddr_t *dest, int packet_metric) ///aggiorna la link_metric
{
  link_metric_t *metricp;
  link_metric_t recorded_metric, new_metric;
  unsigned long time;

/** void *neighbor_attr_get_data(struct neighbor_attr *, const rimeaddr_t *addr) = get pointer to neighbor table data specified by id*/
  metricp = (link_metric_t *)neighbor_attr_get_data(&attr_etx, dest);
  //PRINTF("packet_metric_prima (valore intero) = %d\n", packet_metric); vale 1
  packet_metric = NEIGHBOR_INFO_ETX2FIX(packet_metric);
  //PRINTF("packet_metric_dopo (punto fisso) = %d\n", packet_metric); vale 16
  
  if(metricp == NULL || *metricp == 0) 
  { 
    recorded_metric = NEIGHBOR_INFO_ETX2FIX(ETX_LIMIT); //15*16 = 240
  
///NB:quando si scopre un nuovo vicino con il primo pck che si invia, si inizializza ETX con packet_metric = 16, che è il valore minimo
    new_metric = packet_metric;
   // PRINTF("link_metric con metricp = 0 vale %d\n", new_metric); ///vale 16
  } 
  
  else 
  {
    recorded_metric = *metricp; //vale 16
    //PRINTF("recorded_metric = %d\n", recorded_metric);
    /// Update the EWMA of the ETX for the neighbor    
    ///etx_update = 0.9 * etx_recorded_until_now + (0.1) * etx_last
    new_metric = ((uint16_t)recorded_metric * ETX_ALPHA +
               (uint16_t)packet_metric * (ETX_SCALE - ETX_ALPHA)) / ETX_SCALE;
    PRINTF("link_metric con metricp = 16, quindi dopo EWMA vale %d\n", new_metric);
  }
  //I dati vengono convertiti da un valore di punto fisso a intero:
 /* PRINTF("neighbor-info: ETX changed from %d to %d (packet ETX = %d) %d\n",
	 NEIGHBOR_INFO_FIX2ETX(recorded_metric), // 240/16 = 15
	 NEIGHBOR_INFO_FIX2ETX(new_metric), // 16/16 = 1 
	 NEIGHBOR_INFO_FIX2ETX(packet_metric),
         dest->u8[7]);*/

  if(neighbor_attr_has_neighbor(dest)) ///Check if a neighbor is already added to the neighbor table
  {
    time = clock_seconds();
    neighbor_attr_set_data(&attr_etx, dest, &new_metric); /** Copy data to neighbor table*/
    if(new_metric != recorded_metric && subscriber_callback != NULL) {
      subscriber_callback(dest, 1, new_metric); ///Subscribe to notifications of changed neighbor information
    }
    if(subscriber_callback != NULL)
      subscriber_callback(dest, 1,new_metric);
  }
}
/*---------------------------------------------------------------------------*/
/* Function added by RMonica
 * see header (neighbor-info.h) for explanation
 */
/*
void
neighbor_info_other_source_metric_update(const rimeaddr_t * node, int known)
{
  PRINTF("NEIGHBOR INFO: sono dentro la funct di Monica\n");
  link_metric_t *metricp;
  link_metric_t recorded_metric = NEIGHBOR_INFO_ETX2FIX(ETX_LIMIT);
  metricp = (link_metric_t *)neighbor_attr_get_data(&attr_etx, node);
  if (metricp != NULL || *metricp == 0) {
    recorded_metric = *metricp;
  }

  if(neighbor_attr_has_neighbor(node)) {
    if(subscriber_callback != NULL) {
      subscriber_callback(node, known, recorded_metric);
    }
  }
}*/
/*---------------------------------------------------------------------------*/
static void
add_neighbor(const rimeaddr_t *addr)
{
  switch(neighbor_attr_add_neighbor(addr)) {
  case -1:
    //PRINTF("neighbor-info: failed to add a node.\n");
    break;
  case 0:
    PRINTF("neighbor-info: The neighbor is already known\n");
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
void
neighbor_info_packet_sent(int status, int numtx) ///Notify the neighbor information module about the status of a pck transmission
{
  const rimeaddr_t *dest;
  link_metric_t packet_metric;
#if UIP_DS6_LL_NUD
  uip_ds6_nbr_t *nbr;
#endif /* UIP_DS6_LL_NUD */

  dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(rimeaddr_cmp(dest, &rimeaddr_null)) {
    return;
  }
  
 packet_metric = numtx; //numtx vale 1

  /*PRINTF("neighbor-info: packet sent to %d.%d, status=%d, metric=%u\n",
	dest->u8[sizeof(*dest) - 2], dest->u8[sizeof(*dest) - 1],
	status, (unsigned)packet_metric);*/

PRINTF("neighbor-info: packet sent to %d.%d, status=%d\n",
	dest->u8[sizeof(*dest) - 2], dest->u8[sizeof(*dest) - 1],
	status);
	
  switch(status) {
  case MAC_TX_OK:
    add_neighbor(dest);
#if UIP_DS6_LL_NUD
    nbr = uip_ds6_nbr_ll_lookup((uip_lladdr_t *)dest);
    if(nbr != NULL &&
       (nbr->state == STALE || nbr->state == DELAY || nbr->state == PROBE)) {
      nbr->state = REACHABLE;
      stimer_set(&nbr->reachable, UIP_ND6_REACHABLE_TIME / 1000);
      PRINTF("neighbor-info : received a link layer ACK : ");
      PRINTLLADDR((uip_lladdr_t *)dest);
      PRINTF(" is reachable.\n");
    }
#endif /* UIP_DS6_LL_NUD */
    break;
  case MAC_TX_NOACK:
    packet_metric = ETX_NOACK_PENALTY;
    break;
  default:
    /* Do not penalize the ETX when collisions or transmission
       errors occur. */
    return;
  }
  //PRINTF("Neighbor-info: update metric\n");
  update_metric(dest, packet_metric);
}
/*---------------------------------------------------------------------------*/
void
neighbor_info_packet_received(void)
{
  const rimeaddr_t *src;

  src = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  if(rimeaddr_cmp(src, &rimeaddr_null)) {
    return;
  }

  PRINTF("neighbor-info: packet received from %d.%d\n",
	src->u8[sizeof(*src) - 2], src->u8[sizeof(*src) - 1]);

  add_neighbor(src);
}
/*---------------------------------------------------------------------------*/
int
neighbor_info_subscribe(neighbor_info_subscriber_t s)
{
  if(subscriber_callback == NULL) {
    neighbor_attr_register(&attr_etx);
    neighbor_attr_register(&attr_timestamp);
    subscriber_callback = s;
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
link_metric_t
neighbor_info_get_metric(const rimeaddr_t *addr)
{
  link_metric_t *metricp;

  metricp = (link_metric_t *)neighbor_attr_get_data(&attr_etx, addr);
  return metricp == NULL ? ETX_LIMIT : *metricp;
}
/*---------------------------------------------------------------------------*/
