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
 * $Id: powertrace.c,v 1.8 2010/10/06 18:40:21 adamdunkels Exp $
 */

/**
 * \file
 *         Powertrace: periodically print out power consumption
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/*Modified by Fabio Demicheli:

- Created an API to transmit the energy consumption of a node to the script rpl-of-etx.c
**/

#include "contiki.h"
#include "contiki-lib.h"
#include "sys/compower.h"
#include "powertrace.h"
#include "net/rime.h"

#include <stdio.h>
#include <string.h>

struct powertrace_sniff_stats {
  struct powertrace_sniff_stats *next;
  uint32_t num_input, num_output;
  uint32_t input_txtime, input_rxtime;
  uint32_t output_txtime, output_rxtime;
#if UIP_CONF_IPV6
  uint16_t proto; /* includes proto + possibly flags */
#endif
  uint16_t channel;
  uint32_t last_input_txtime, last_input_rxtime;
  uint32_t last_output_txtime, last_output_rxtime;
};

#define INPUT  1
#define OUTPUT 0

#define MAX_NUM_STATS  16

MEMB(stats_memb, struct powertrace_sniff_stats, MAX_NUM_STATS);
LIST(stats_list);

PROCESS(powertrace_process, "Periodic power output");
/*---------------------------------------------------------------------------*/
void
powertrace_print(char *str)
{
  static uint32_t last_cpu, last_lpm, last_transmit, last_listen;
  static uint32_t last_idle_transmit, last_idle_listen;

  uint32_t cpu, lpm, transmit, listen;
  uint32_t all_cpu, all_lpm, all_transmit, all_listen;
  uint32_t idle_transmit, idle_listen;
  uint32_t all_idle_transmit, all_idle_listen;
  
  static uint32_t seqno;

  uint32_t time, all_time, radio, all_radio;

  uint32_t energy_consumed_node_mJ;// variable added by FDemicheli
  uint32_t energy_consumed_node_rx, energy_consumed_node_tx;// variable added by FDemicheli
  
  struct powertrace_sniff_stats *s;

  energest_flush();
  ///energest_type_time = return integer value. It is an unsigned long ed is expressed in rtimer ticks.
  ///Remember: to obtain seconds, I have to divide by RTIMER_ARCH_SECOND = 32768
  ///These are the total time values, wich powertrace sum every time
  all_cpu = energest_type_time(ENERGEST_TYPE_CPU);///is high power CPU time
  all_lpm = energest_type_time(ENERGEST_TYPE_LPM);///is low power CPU time. It uses 3% of full power and is activated when no processes 
                                                 ///have run and there are no events pending
  all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  all_idle_transmit = compower_idle_activity.transmit;///always 0
  all_idle_listen = compower_idle_activity.listen;
  
  //printf("all_idle_listen = %lu\n",all_idle_listen);
  //printf("all_transmit = %lu\n", all_transmit);
  //printf("all_listen = %lu\n",all_listen);
  
  //printf("\n");
  
 // printf("all_idle_listen in sec = %lu\n",all_idle_listen/RTIMER_ARCH_SECOND);
  //printf("all_transmit in sec = %lu\n", all_transmit/RTIMER_ARCH_SECOND);
  //printf("all_listen in sec = %lu\n",all_listen/RTIMER_ARCH_SECOND);
  
  ///Compute Tm,n = time during which component m has been in state n --> see Powertrace paper page 5
 
  ///Current values obtained from Tmote-sky datasheet. Tension V = 3 V.
  ///CPU = the component leave high power CPU state. MCU ON, Radio OFF. Current = 1.8 mA
  ///LPM = the component leave low power CPU state. MCU IDLE, Radio OFF. Current = 0.545 mA
 ///TRANSMIT = MCU ON, radio TX. Current = 19.5mA --> 20 mA
 ///LISTEN = MCU ON, radio RX. Current = 21.8mA --> 22 mA
 
  cpu = all_cpu - last_cpu;
  lpm = all_lpm - last_lpm;
  
  transmit = all_transmit - last_transmit;
  listen = all_listen - last_listen;
  
  idle_transmit = compower_idle_activity.transmit - last_idle_transmit;///always 0
  idle_listen = compower_idle_activity.listen - last_idle_listen;///What current value for idle_listen??? The same of listen??
  
  /** FDemicheli
  The energy cost is P= V * I and E = P * Tm.n --> from Energest paper
     --> E = V * I * Tm,n = (3 * 20) * T_transmit + (3 * 22) * T_listen + (3 * ??) * T_idlelisten [mJ]
     where
      T_transmit = all_transmit / RTIMER_ARCH_SECOND --> rtimer ticks * sec /rtimer ticks = sec
      T_listen = all_listen / RTIMER_ARCH_SECOND
      T_idlelisten = all_idle_listen / RTIMER_ARCH_SECOND
  
  The computation is corrects because I multiple (V * mA) * sec = mW * sec = mJ
  
  */
  
  ///The same values as above
  last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  last_listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  last_idle_listen = compower_idle_activity.listen;
  last_idle_transmit = compower_idle_activity.transmit;///always 0
  
  radio = transmit + listen;
  time = cpu + lpm;
  all_time = all_cpu + all_lpm;
  all_radio = energest_type_time(ENERGEST_TYPE_LISTEN) +
    energest_type_time(ENERGEST_TYPE_TRANSMIT);
    
  energy_consumed_node_rx = 3 * 22 * (all_listen / RTIMER_ARCH_SECOND);
  
  printf("energy_consumed_node_rx = %lu\n",energy_consumed_node_rx);
  
  energy_consumed_node_tx  = 3 * 20 * (all_transmit/RTIMER_ARCH_SECOND);
  
  printf("energy_consumed_node_tx = %lu\n",energy_consumed_node_tx);
  
  energy_consumed_node_mJ = energy_consumed_node_tx + energy_consumed_node_rx;
    
  printf("energy_consumed_node_mJ = %lu\n",energy_consumed_node_mJ);
  
    
 /* printf("%s %lu P %d.%d %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu (radio %d.%02d%% / %d.%02d%% tx %d.%02d%% / %d.%02d%% listen %d.%02d%% / %d.%02d%%)\n",
         str,
         clock_time(), rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], seqno,
         all_cpu, all_lpm, all_transmit, all_listen, all_idle_transmit, all_idle_listen,
         cpu, lpm, transmit, listen, idle_transmit, idle_listen,
         (int)((100L * (all_transmit + all_listen)) / all_time),
         (int)((10000L * (all_transmit + all_listen) / all_time) - (100L * (all_transmit + all_listen) / all_time) * 100),
         (int)((100L * (transmit + listen)) / time),
         (int)((10000L * (transmit + listen) / time) - (100L * (transmit + listen) / time) * 100),
         (int)((100L * all_transmit) / all_time),
         (int)((10000L * all_transmit) / all_time - (100L * all_transmit / all_time) * 100),
         (int)((100L * transmit) / time),
         (int)((10000L * transmit) / time - (100L * transmit / time) * 100),
         (int)((100L * all_listen) / all_time),
         (int)((10000L * all_listen) / all_time - (100L * all_listen / all_time) * 100),
         (int)((100L * listen) / time),
         (int)((10000L * listen) / time - (100L * listen / time) * 100));*/
 /*Da usare x il file Matlab
  printf("%s %lu P %d.%d %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
         str,
         clock_time(), rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], seqno,
         all_cpu, all_lpm, all_transmit, all_listen, all_idle_transmit, all_idle_listen,
         cpu, lpm, transmit, listen, idle_transmit, idle_listen,all_time,time);*/

  for(s = list_head(stats_list); s != NULL; s = list_item_next(s)) {

#if ! UIP_CONF_IPV6
   /* printf("%s %lu SP %d.%d %lu %u %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu (channel %d radio %d.%02d%% / %d.%02d%%)\n",
           str, clock_time(), rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], seqno,
           s->channel,
           s->num_input, s->input_txtime, s->input_rxtime,
           s->input_txtime - s->last_input_txtime,
           s->input_rxtime - s->last_input_rxtime,
           s->num_output, s->output_txtime, s->output_rxtime,
           s->output_txtime - s->last_output_txtime,
           s->output_rxtime - s->last_output_rxtime,
           s->channel,
           (int)((100L * (s->input_rxtime + s->input_txtime + s->output_rxtime + s->output_txtime)) / all_radio),
           (int)((10000L * (s->input_rxtime + s->input_txtime + s->output_rxtime + s->output_txtime)) / all_radio),
           (int)((100L * (s->input_rxtime + s->input_txtime +
                          s->output_rxtime + s->output_txtime -
                          (s->last_input_rxtime + s->last_input_txtime +
                           s->last_output_rxtime + s->last_output_txtime))) /
                 radio),
           (int)((10000L * (s->input_rxtime + s->input_txtime +
                          s->output_rxtime + s->output_txtime -
                          (s->last_input_rxtime + s->last_input_txtime +
                           s->last_output_rxtime + s->last_output_txtime))) /
                 radio));*/
#else
  /*  printf("%s %lu SP %d.%d %lu %u %u %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu (proto %u(%u) radio %d.%02d%% / %d.%02d%%)\n",
           str, clock_time(), rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], seqno,
           s->proto, s->channel,
           s->num_input, s->input_txtime, s->input_rxtime,
           s->input_txtime - s->last_input_txtime,
           s->input_rxtime - s->last_input_rxtime,
           s->num_output, s->output_txtime, s->output_rxtime,
           s->output_txtime - s->last_output_txtime,
           s->output_rxtime - s->last_output_rxtime,
           s->proto, s->channel,
           (int)((100L * (s->input_rxtime + s->input_txtime + s->output_rxtime + s->output_txtime)) / all_radio),
           (int)((10000L * (s->input_rxtime + s->input_txtime + s->output_rxtime + s->output_txtime)) / all_radio),
           (int)((100L * (s->input_rxtime + s->input_txtime +
                          s->output_rxtime + s->output_txtime -
                          (s->last_input_rxtime + s->last_input_txtime +
                           s->last_output_rxtime + s->last_output_txtime))) /
                 radio),
           (int)((10000L * (s->input_rxtime + s->input_txtime +
                          s->output_rxtime + s->output_txtime -
                          (s->last_input_rxtime + s->last_input_txtime +
                           s->last_output_rxtime + s->last_output_txtime))) /
                 radio));*/
#endif
    s->last_input_txtime = s->input_txtime;
    s->last_input_rxtime = s->input_rxtime;
    s->last_output_txtime = s->output_txtime;
    s->last_output_rxtime = s->output_rxtime;
    
  }
  seqno++;
}


/*-------------------------------------------------------------------------------------------------------------------------*/
//Function added by Fabio Demicheli. Declared in sys/energest.h line 77
uint16_t 
energest_get_current_energy_consumption()
{ 
  uint32_t energy_consumed_node_mJ;
  uint32_t all_transmit, all_listen;
  energest_flush();
  all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);  
 // printf("all_transmit_function = %lu\n", all_transmit);
 // printf("all_listen_function = %lu\n",all_listen);
  energy_consumed_node_mJ = 3 * 20 * (all_transmit/RTIMER_ARCH_SECOND) + 3 * 22 * (all_listen / RTIMER_ARCH_SECOND);
  //printf("energest_cons = %lu\n",energy_consumed_node_mJ);
  return energy_consumed_node_mJ;
}
/*--------------------------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(powertrace_process, ev, data)
{
  static struct etimer periodic;
  clock_time_t *period;
  PROCESS_BEGIN();

  period = data;

  if(period == NULL) {
    PROCESS_EXIT();
  }
  etimer_set(&periodic, *period);

  while(1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic));
    etimer_reset(&periodic);
    powertrace_print("");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
powertrace_start(clock_time_t period)
{
  process_start(&powertrace_process, (void *)&period);
}
/*---------------------------------------------------------------------------*/
void
powertrace_stop(void)
{
  process_exit(&powertrace_process);
}
/*---------------------------------------------------------------------------*/
static void
add_stats(struct powertrace_sniff_stats *s, int input_or_output)
{
  if(input_or_output == INPUT) {
    s->num_input++;
    s->input_txtime += packetbuf_attr(PACKETBUF_ATTR_TRANSMIT_TIME);
    s->input_rxtime += packetbuf_attr(PACKETBUF_ATTR_LISTEN_TIME);
  } else if(input_or_output == OUTPUT) {
    s->num_output++;
    s->output_txtime += packetbuf_attr(PACKETBUF_ATTR_TRANSMIT_TIME);
    s->output_rxtime += packetbuf_attr(PACKETBUF_ATTR_LISTEN_TIME);
  }
}
/*---------------------------------------------------------------------------*/
static void
add_packet_stats(int input_or_output)
{
  struct powertrace_sniff_stats *s;

  /* Go through the list of stats to find one that matches the channel
     of the packet. If we don't find one, we allocate a new one and
     put it on the list. */
  for(s = list_head(stats_list); s != NULL; s = list_item_next(s)) {
    if(s->channel == packetbuf_attr(PACKETBUF_ATTR_CHANNEL)
#if UIP_CONF_IPV6
       && s->proto == packetbuf_attr(PACKETBUF_ATTR_NETWORK_ID)
#endif
       ) {
      add_stats(s, input_or_output);
      break;
    }
  }
  if(s == NULL) {
    s = memb_alloc(&stats_memb);
    if(s != NULL) {
      memset(s, 0, sizeof(struct powertrace_sniff_stats));
      s->channel = packetbuf_attr(PACKETBUF_ATTR_CHANNEL);
#if UIP_CONF_IPV6
      s->proto = packetbuf_attr(PACKETBUF_ATTR_NETWORK_ID);
#endif
      list_add(stats_list, s);
      add_stats(s, input_or_output);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
input_sniffer(void)
{
  add_packet_stats(INPUT);
}
/*---------------------------------------------------------------------------*/
static void
output_sniffer(int mac_status)
{
  add_packet_stats(OUTPUT);
}
/*---------------------------------------------------------------------------*/
#if ! UIP_CONF_IPV6
static void
sniffprint(char *prefix, int seqno)
{
  const rimeaddr_t *sender, *receiver, *esender, *ereceiver;

  sender = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  receiver = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  esender = packetbuf_addr(PACKETBUF_ADDR_ESENDER);
  ereceiver = packetbuf_addr(PACKETBUF_ADDR_ERECEIVER);


  printf("%lu %s %d %u %d %d %d.%d %u %u\n",
         clock_time(),
         prefix,
         rimeaddr_node_addr.u8[0], seqno,
         packetbuf_attr(PACKETBUF_ATTR_CHANNEL),
         packetbuf_attr(PACKETBUF_ATTR_PACKET_TYPE),
         esender->u8[0], esender->u8[1],
         packetbuf_attr(PACKETBUF_ATTR_TRANSMIT_TIME),
         packetbuf_attr(PACKETBUF_ATTR_LISTEN_TIME));
}
/*---------------------------------------------------------------------------*/
static void
input_printsniffer(void)
{
  static int seqno = 0; 
  sniffprint("I", seqno++);

  if(packetbuf_attr(PACKETBUF_ATTR_CHANNEL) == 0) {
    int i;
    uint8_t *dataptr;

    printf("x %d ", packetbuf_totlen());
    dataptr = packetbuf_hdrptr();
    printf("%02x ", dataptr[0]);
    for(i = 1; i < packetbuf_totlen(); ++i) {
      printf("%02x ", dataptr[i]);
    }
    printf("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
output_printsniffer(int mac_status)
{
  static int seqno = 0;
  sniffprint("O", seqno++);
}
/*---------------------------------------------------------------------------*/
RIME_SNIFFER(printsniff, input_printsniffer, output_printsniffer);
/*---------------------------------------------------------------------------*/
void
powertrace_printsniff(powertrace_onoff_t onoff)
{
  switch(onoff) {
  case POWERTRACE_ON:
    rime_sniffer_add(&printsniff);
    break;
  case POWERTRACE_OFF:
    rime_sniffer_remove(&printsniff);
    break;
  }
}
#endif
/*---------------------------------------------------------------------------*/
RIME_SNIFFER(powersniff, input_sniffer, output_sniffer);
/*---------------------------------------------------------------------------*/
void
powertrace_sniff(powertrace_onoff_t onoff)
{
  switch(onoff) {
  case POWERTRACE_ON:
    rime_sniffer_add(&powersniff);
    break;
  case POWERTRACE_OFF:
    rime_sniffer_remove(&powersniff);
    break;
  }
}
