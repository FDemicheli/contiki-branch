/**
 * \defgroup cc1100 CC1100
 * CC1100 radio driver
 * @{
 *
 * \file
 * CC1100 radio driver code
 *
 * \author Alexandre Boeglin <alexandre.boeglin@inria.fr>
 */

/*
 * Copyright (c) 2010, Institut National de Recherche en Informatique et en Automatique
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
 * $Id: $
 */
/*
 * This code is almost device independent and should be easy to port.
 */

#include <string.h>

#include "contiki.h"

#if defined(__AVR__)
#include <avr/io.h>
#endif

#include "watchdog.h"

#include "dev/leds.h"
#include "dev/spi.h"
#include "dev/cc1100.h"
#include "dev/cc1100_const.h"
#include "dev/cc1100_conf.h"

#include "net/packetbuf.h"
#include "net/rime/rimestats.h"
#include "net/netstack.h"

#include "sys/timetable.h"

#define WITH_SEND_CCA 1

#define PKTLEN_LEN 1
#define FOOTER_LEN 2

/* Local Buffers. */
static uint8_t rx_buffer[CC1100_MAX_PACKET_LEN + FOOTER_LEN];
static uint8_t rx_buffer_len = 0;
static uint8_t tx_buffer[PKTLEN_LEN + CC1100_MAX_PACKET_LEN];
static uint8_t tx_buffer_len = 0;
static uint8_t tx_buffer_ptr = 0;

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#define DEBUG_LEDS DEBUG
#undef LEDS_ON
#undef LEDS_OFF
#if DEBUG_LEDS
#define LEDS_ON(x) leds_on(x)
#define LEDS_OFF(x) leds_off(x)
#else
#define LEDS_ON(x)
#define LEDS_OFF(x)
#endif

extern void cc1100_arch_init(void);

int cc1100_packets_seen, cc1100_packets_read;

#define BUSYWAIT_UNTIL(cond, max_time)                                  \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(1) {                                                          \
      rtimer_clock_t tnow, tmax;                                        \
      if(cond) break;                                                   \
      tnow = RTIMER_NOW();                                              \
      tmax = t0 + (max_time);                                           \
      if(tmax >= t0 && (tnow >= tmax || tnow < t0)) break;              \
      if(tmax < t0 && tnow < t0 && tnow >= tmax) break;                 \
    }                                                                   \
  } while(0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/*---------------------------------------------------------------------------*/
PROCESS(cc1100_process, "CC1100 driver");
/*---------------------------------------------------------------------------*/

int cc1100_on(void);
int cc1100_off(void);

static unsigned int status(void);
static unsigned getreg(enum cc1100_register regname);

static int read_rxfifo();
static int cc1100_read(void *buf, unsigned short bufsize);

static int cc1100_prepare(const void *data, unsigned short len);
static int cc1100_transmit(unsigned short len);
static int cc1100_send(const void *data, unsigned short len);

static int cc1100_receiving_packet(void);
static int pending_packet(void);
static int cc1100_cca(void);
/*static int detected_energy(void);*/

int16_t cc1100_last_rssi;
uint8_t cc1100_last_lqi;

const struct radio_driver cc1100_driver =
  {
    cc1100_init,
    cc1100_prepare,
    cc1100_transmit,
    cc1100_send,
    cc1100_read,
    /* cc1100_set_channel, */
    /* detected_energy, */
    cc1100_cca,
    cc1100_receiving_packet,
    pending_packet,
    cc1100_on,
    cc1100_off,
  };

static uint8_t receive_on;

static int channel;

/*---------------------------------------------------------------------------*/

static void
getrxdata(void *buf, int len)
{
  CC1100_READ_FIFO_BUF(buf, len);
}
static void
getrxbyte(uint8_t *byte)
{
  CC1100_READ_FIFO_BYTE(*byte);
}
static void
flushrx(void)
{
  uint8_t state = status() & CC1100_STATE_MASK;
  if(state == CC1100_STATE_IDLE || state == CC1100_STATE_RXFIFO_OVERFLOW) {
    CC1100_STROBE(CC1100_SFRX);
  } else {
    PRINTF("CC1100 Error: flushrx while not in IDLE or RXFIFO_OVERFLOW!\n");
  }
}
static void
flushtx(void)
{
  uint8_t state = status() & CC1100_STATE_MASK;
  if(state == CC1100_STATE_IDLE || state == CC1100_STATE_TXFIFO_UNDERFLOW) {
    CC1100_STROBE(CC1100_SFTX);
  } else {
    PRINTF("CC1100 Error: flushtx while not in IDLE or TXFIFO_UNDERFLOW!\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
strobe(enum cc1100_register regname)
{
  CC1100_STROBE(regname);
}
/*---------------------------------------------------------------------------*/
static unsigned int
status(void)
{
  uint8_t status;
  CC1100_GET_STATUS(status);
  return status;
}
/*---------------------------------------------------------------------------*/
static uint8_t locked, lock_on, lock_off;

static void
on(void)
{
  CC1100_ENABLE_GDO0_INT();
  strobe(CC1100_SRX);

  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) == CC1100_STATE_RX, RTIMER_SECOND / 100);
  if((status() & CC1100_STATE_MASK) != CC1100_STATE_RX) {
    PRINTF("CC1100 Error: could not get into RX! %02x - %d\n", status(), getreg(CC1100_MARCSTATE));
  }

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  LEDS_ON(LEDS_BLUE);
  receive_on = 1;
}
static void
off(void)
{
  receive_on = 0;

  /* Wait for transmission to end before turning radio off. */
  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) != CC1100_STATE_TX, RTIMER_SECOND / 100);

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  strobe(CC1100_SIDLE);
  CC1100_DISABLE_GDO0_INT();
  LEDS_OFF(LEDS_BLUE);

  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) == CC1100_STATE_IDLE, RTIMER_SECOND / 100);
  if((status() & CC1100_STATE_MASK) != CC1100_STATE_IDLE) {
    PRINTF("CC1100 Error: could not get into IDLE! %02x - %d\n", status(), getreg(CC1100_MARCSTATE));
  }

  /* Flush FIFOs. */
  flushrx();
  flushtx();
}
/*---------------------------------------------------------------------------*/
#define GET_LOCK() locked++
static void RELEASE_LOCK(void) {
  if(locked == 1) {
    if(lock_on) {
      on();
      lock_on = 0;
    }
    if(lock_off) {
      off();
      lock_off = 0;
    }
  }
  locked--;
}
/*---------------------------------------------------------------------------*/
static unsigned
getreg(enum cc1100_register regname)
{
  unsigned reg;
  CC1100_READ_REG(regname, reg);
  return reg;
}
/*---------------------------------------------------------------------------*/
static void
setreg(enum cc1100_register regname, unsigned value)
{
  CC1100_WRITE_REG(regname, value);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Set the transmission power.
 * \param power The transmission power in dB.
 */
static void
set_txpower(int8_t power)
{
  uint8_t pa_value;
  int i;
  int found = 0;

  for(i = 0; i < sizeof(CC1100_PA_LEVELS) / sizeof(CC1100_PA_LEVELS[0]); ++i) {
    if(CC1100_PA_LEVELS[i].level <= power) {
      pa_value = CC1100_PA_LEVELS[i].value;
      found = 1;
      break;
    }
  }

  /* Requested value was too small, use the lowest known value. */
  if(!found) {
    pa_value = CC1100_PA_LEVELS[i - 1].value;
  }

  setreg(CC1100_PATABLE, pa_value);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialize the radio.
 */
int
cc1100_init(void)
{
  {
    int s = splhigh();
    cc1100_arch_init();		/* Initalize ports and SPI. */
    CC1100_DISABLE_GDO0_INT();
    CC1100_GDO0_INT_INIT();
    splx(s);
  }

  /* Manual Reset Sequence. */
  CC1100_CSN_PORT(OUT) &= ~BV(CC1100_CSN_PIN); /* Strobe CSn low / high, wait for 40 us. */
  CC1100_CSN_PORT(OUT) |= BV(CC1100_CSN_PIN);
  BUSYWAIT_UNTIL(0, MAX(RTIMER_SECOND / 25000, 1));
  strobe(CC1100_SRES);

  /* Change config register values as recomended by SmartRF Studio. */
  CC1100_WRITE_BURST(CC1100_IOCFG2, CC1100_CONFIG_REGISTERS, sizeof(CC1100_CONFIG_REGISTERS) / sizeof(CC1100_CONFIG_REGISTERS[0]));

  /* Change PA Table values as recomended by SmartRF Studio. */
  CC1100_WRITE_BURST(CC1100_PATABLE, CC1100_PA_TABLE, sizeof(CC1100_PA_TABLE) / sizeof(CC1100_PA_TABLE[0]));

  /* Set the FIFO threshold to minimum. */
  setreg(CC1100_FIFOTHR, 0x00);

  /* Set max packet length. */
  setreg(CC1100_PKTLEN, PKTLEN_LEN + CC1100_MAX_PACKET_LEN);

  cc1100_set_channel(26);

  /*
   * Do an initial calibration.
   * Required to be able to receive with FS_AUTOCAL != 1.
   */
  strobe(CC1100_SCAL);
  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) == CC1100_STATE_IDLE, RTIMER_SECOND / 100);

  process_start(&cc1100_process, NULL);
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Send the packet that has previously been prepared.
 * \param payload_len The packet length.
 * \return The transmission result status.
 * \return RADIO_TX_OK Transmission succeeded.
 * \return RADIO_TX_COLLISION Transmission failed: the cahnnel wasn't clear.
 * \return RADIO_TX_ERR Transmission failed: FIFO error.
 */
static int
cc1100_transmit(unsigned short payload_len)
{
  int txpower;
  int ret = RADIO_TX_OK; /* Default return value. */

  GET_LOCK();

  txpower = 0;
  if(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
    /* Remember the current transmission power. */
    txpower = cc1100_get_txpower();
    /* Set the specified transmission power. */
    set_txpower(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) - 1);
  }

  /*
   * The TX FIFO can only hold one packet. Make sure to not overrun
   * FIFO by waiting for transmission to start here and synchronizing
   * with the CC1100_TX_ACTIVE check in cc1100_send.
   */
#if WITH_SEND_CCA
  if(!receive_on) {
    strobe(CC1100_SRX);
  }
  /* Set GDO2 as CCA. */
  setreg(CC1100_IOCFG2, CC1100_GDO_CCA);
  BUSYWAIT_UNTIL(CC1100_GDO2_IS_1, RTIMER_SECOND / 100);
  strobe(CC1100_STX);
  /* Reset GDO2 as SFD. */
  setreg(CC1100_IOCFG2, CC1100_GDO_SYNCWORD);
#else /* WITH_SEND_CCA */
  strobe(CC1100_STX);
#endif /* WITH_SEND_CCA */

  switch(status() & CC1100_STATE_MASK) {
    case CC1100_STATE_RXFIFO_OVERFLOW:
      PRINTF("CC1100 Error: RXFIFO overflowed while checking for CCA!\n");
      flushrx();
      if(receive_on) {
	strobe(CC1100_SRX);
      }
    case CC1100_STATE_RX:
      /*
       * We are still in RX. This means that the channel is not clear,
       * so we drop the transmission.
       */
      RIMESTATS_ADD(contentiondrop);

      if(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
        /* Restore the transmission power. */
        set_txpower(txpower & 0xff);
      }
      if(!receive_on) {
        strobe(CC1100_SIDLE);
      }

      PRINTF("CC1100 Error: channel not clear for TX!\n");
      RELEASE_LOCK();
      return RADIO_TX_COLLISION;
  }

  if(receive_on) {
    ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  }

  LEDS_ON(LEDS_RED);

  ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

  setreg(CC1100_IOCFG2, CC1100_GDO_TXFIFO);
  while(tx_buffer_ptr < tx_buffer_len) {
    if((status() & CC1100_STATE_MASK) == CC1100_STATE_TXFIFO_UNDERFLOW) {
      PRINTF("CC1100 Error: TXFIFO underflowed!\n");
      flushtx();
      ret = RADIO_TX_ERR;
      break;
    }
    /*
     * Only read CC1100_TXBYTES if CC1100_GDO_TXFIFO is not set.
     * MIN is here to prevent "SPI Read Synchronization Issue" (CC1101 Errata 3).
     */
    uint8_t available_tx = CC1100_GDO2_IS_1 ? 0 : 64 - MIN(getreg(CC1100_TXBYTES) & 0x7F, 64);
    int write_len = MIN(tx_buffer_len - tx_buffer_ptr, available_tx);
    if(write_len) {
      CC1100_WRITE_FIFO_BUF(tx_buffer + tx_buffer_ptr, write_len);
      tx_buffer_ptr += write_len;
    }

    /* Add a small delay, in order not to flood the radio with status requests. */
    clock_delay(100);
  }
  setreg(CC1100_IOCFG2, CC1100_GDO_SYNCWORD);

  /*
   * We wait until transmission has ended so that we get an
   * accurate measurement of the transmission time.
   */
  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) != CC1100_STATE_TX, RTIMER_SECOND / 100);

#ifdef ENERGEST_CONF_LEVELDEVICE_LEVELS
  ENERGEST_OFF_LEVEL(ENERGEST_TYPE_TRANSMIT,cc1100_get_txpower());
#endif
  ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

  LEDS_OFF(LEDS_RED);

  /* Wait for a possible calibration to complete (FS_AUTOCAL = 3). */
  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) == CC1100_STATE_IDLE, RTIMER_SECOND / 100);

  /* Just to make sure we don't have garbage from CCA. */
  flushrx();

  if(receive_on) {
    /* We need to explicitly turn the radio on again, since TXOFF_MODE is IDLE. */
    on();
  } else {
    off();
  }

  if(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
    /* Restore the transmission power. */
    set_txpower(txpower & 0xff);
  }

  RELEASE_LOCK();
  return ret;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Prepare the radio with a packet to be sent.
 * \param payload The packet.
 * \param payload_len The packet length.
 */
static int
cc1100_prepare(const void *payload, unsigned short payload_len)
{
  GET_LOCK();

  /* This is a hack, to prevent tx_buffer overflow. */
  if(payload_len > CC1100_MAX_PACKET_LEN) {
    PRINTF("CC1100 Error: trying to send %d, CC1100_MAX_PACKET_LEN is %d!\n", payload_len, CC1100_MAX_PACKET_LEN);
    payload_len = MIN(payload_len, CC1100_MAX_PACKET_LEN);
  }

  RIMESTATS_ADD(lltx);

  /* Write packet to TX FIFO. */
  tx_buffer_len = payload_len + PKTLEN_LEN;
  tx_buffer_ptr = 0;

  tx_buffer[0] = payload_len;
  memcpy(tx_buffer + PKTLEN_LEN, payload, payload_len);

  RELEASE_LOCK();
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Prepare & transmit a packet.
 * \param payload The packet.
 * \param payload_len The packet length.
 * \return The transmission result status.
 * \return RADIO_TX_OK Transmission succeeded.
 * \return RADIO_TX_COLLISION Transmission failed: the cahnnel wasn't clear.
 * \return RADIO_TX_ERR Transmission failed: FIFO error.
 */
static int
cc1100_send(const void *payload, unsigned short payload_len)
{
  cc1100_prepare(payload, payload_len);
  return cc1100_transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Turn the radio off.
 */
int
cc1100_off(void)
{
  /* Don't do anything if we are already turned off. */
  if(receive_on == 0) {
    return 1;
  }

  /*
   * If we are called when the driver is locked, we indicate that the
   * radio should be turned off when the lock is unlocked.
   */
  if(locked) {
    lock_off = 1;
    return 1;
  }

  GET_LOCK();
  /*
   * If we are currently receiving a packet (indicated by SFD == 1),
   * we don't actually switch the radio off now, but signal that the
   * driver should switch off the radio once the packet has been
   * received and processed, by setting the 'lock_off' variable.
   */
  if(CC1100_GDO2_IS_1) {
    lock_off = 1;
  } else {
    off();
  }
  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Turn the radio on.
 */
int
cc1100_on(void)
{
  if(receive_on) {
    return 1;
  }
  if(locked) {
    lock_on = 1;
    return 1;
  }

  GET_LOCK();
  on();
  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Return the current radio channel.
 * \return The current radio channel.
 */
int
cc1100_get_channel(void)
{
  return channel;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Set the radio channel.
 * \param c The radio channel.
 * \return The channel switch result status.
 * \return 1 Channel switch succeeded.
 * \return 0 Channel switch failed.
 */
int
cc1100_set_channel(int c)
{
  GET_LOCK();

  BUSYWAIT_UNTIL((status() & CC1100_STATE_MASK) == CC1100_STATE_IDLE, RTIMER_SECOND / 100);
  if((status() & CC1100_STATE_MASK) != CC1100_STATE_IDLE) {
    PRINTF("CC1100 Error: channel can only be changed while in IDLE state!\n");
    RELEASE_LOCK();
    return 0;
  }

  channel = c;

  setreg(CC1100_CHANNR, c);

  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
#if CC1100_TIMETABLE_PROFILING
#define cc1100_timetable_size 16
TIMETABLE(cc1100_timetable);
TIMETABLE_AGGREGATE(aggregate_time, 10);
#endif /* CC1100_TIMETABLE_PROFILING */
/**
 * \brief Received packet interrupt handler.
 *
 * Triggered when the RXFIFO starts to fill.
 */
int
cc1100_interrupt(void)
{
  LEDS_ON(LEDS_GREEN);

  if(read_rxfifo()) {
    process_poll(&cc1100_process);
    cc1100_packets_seen++;
  }

  CC1100_CLEAR_GDO0_INT();

#if CC1100_TIMETABLE_PROFILING
  timetable_clear(&cc1100_timetable);
  TIMETABLE_TIMESTAMP(cc1100_timetable, "interrupt");
#endif /* CC1100_TIMETABLE_PROFILING */

  LEDS_OFF(LEDS_GREEN);
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Forward received packets to upper layer.
 */
PROCESS_THREAD(cc1100_process, ev, data)
{
  int len;
  PROCESS_BEGIN();

  PRINTF("cc1100_process: started\n");

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
#if CC1100_TIMETABLE_PROFILING
    TIMETABLE_TIMESTAMP(cc1100_timetable, "poll");
#endif /* CC1100_TIMETABLE_PROFILING */

    packetbuf_clear();
    len = cc1100_read(packetbuf_dataptr(), PACKETBUF_SIZE);
    
    packetbuf_set_datalen(len);
    
    NETSTACK_RDC.input();
#if CC1100_TIMETABLE_PROFILING
    TIMETABLE_TIMESTAMP(cc1100_timetable, "end");
    timetable_aggregate_compute_detailed(&aggregate_time,
                                         &cc1100_timetable);
      timetable_clear(&cc1100_timetable);
#endif /* CC1100_TIMETABLE_PROFILING */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Read a frame from RXFIFO and write it to the buffer,
 * discarding any previously read frame.
 * \return The packet length.
 * \return 0 In case of error.
 */
static int
read_rxfifo()
{
  uint8_t len;
  int remaining_len;
  uint8_t previous_gdo2 = getreg(CC1100_IOCFG2);

  uint8_t cur_status = status();
  if(!(cur_status & CC1100_FIFO_BYTES_AVAILABLE_MASK)) {
    return 0;
  } else if((cur_status & CC1100_STATE_MASK) == CC1100_STATE_IDLE) {
    PRINTF("CC1100 Error: went IDLE while entering interrupt!\n");
    return 0;
  }

  getrxbyte(&len);
  remaining_len = len + FOOTER_LEN;

  if(len > CC1100_MAX_PACKET_LEN) {
    /* Oops, we must be out of sync. */
    strobe(CC1100_SIDLE);
    flushrx();
    strobe(CC1100_SRX);
    RIMESTATS_ADD(badsynch);
    return 0;
  }

  if(len < 1) {
    strobe(CC1100_SIDLE);
    flushrx();
    strobe(CC1100_SRX);
    RIMESTATS_ADD(tooshort);
    return 0;
  }

  /* Set GDO2 as SFD. */
  if(previous_gdo2 != CC1100_GDO_SYNCWORD) {
    setreg(CC1100_IOCFG2, CC1100_GDO_SYNCWORD);
  }

  /* Restart the watchdog, to make sure it isn't triggered due to too many received packets. */
  watchdog_periodic();

  while(remaining_len && (CC1100_GDO2_IS_1 || CC1100_GDO0_IS_1)) {
    if((status() & CC1100_STATE_MASK) == CC1100_STATE_RXFIFO_OVERFLOW) {
      PRINTF("CC1100 Error: RXFIFO overflowed!\n");
      flushrx();
      strobe(CC1100_SRX);
      break;
    }
    /*
     * Only read CC1100_RXBYTES if CC1100_GDO_RXFIFO_EOP is set.
     * MIN is here to prevent "SPI Read Synchronization Issue" (CC1101 Errata 3).
     */
    uint8_t available_rx = CC1100_GDO0_IS_1 ? MIN(getreg(CC1100_RXBYTES) & 0x7F, 64) : 0;
    int read_len;
    if(remaining_len <= available_rx) {
      /* End of packet, read completely. */
      read_len = remaining_len;
    } else {
      /* Packet not yet complete, must leave at least one byte in FIFO. */
      read_len = MAX(available_rx - 1, 0);
    }
    if(read_len) {
      getrxdata(rx_buffer + len + FOOTER_LEN - remaining_len, read_len);
      remaining_len -= read_len;
    }

    /* Add a small delay, in order not to flood the radio with status requests. */
    clock_delay(100);
  }

  /* Restore previous value of GDO2. */
  if(previous_gdo2 != CC1100_GDO_SYNCWORD) {
    setreg(CC1100_IOCFG2, previous_gdo2);
  }

  if(remaining_len) {
    PRINTF("CC1100 Error: RXFIFO underflowed, bogus packet!\n");
    rx_buffer_len = 0;
  } else {
    rx_buffer_len = len;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Read a received packet into a buffer.
 * \param buf The destination buffer.
 * \param bufsize The size of the destination buffer.
 * \return The packet length.
 * \return 0 In case of error.
 */
static int
cc1100_read(void *buf, unsigned short bufsize)
{
  uint8_t len;

  if(!rx_buffer_len) {
    return 0;
  }

  GET_LOCK();

  cc1100_packets_read++;

  len = rx_buffer_len;

  if(len > bufsize) {
    PRINTF("CC1100 Error: packet too long for buffer!\n");
    rx_buffer_len = 0;
    RIMESTATS_ADD(toolong);
    RELEASE_LOCK();
    return 0;
  }

  memcpy(buf, rx_buffer, len);

  if(rx_buffer[len + 1] & 0x80) { /* Packet fully received && CRC OK. */
    cc1100_last_rssi = (int16_t)(signed char)rx_buffer[len] / 2 - 74;
    cc1100_last_lqi = rx_buffer[len + 1] & 0x7f;

    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, cc1100_last_rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, cc1100_last_lqi);

    rx_buffer_len = 0;
    RIMESTATS_ADD(llrx);

  } else {
    PRINTF("CC1100 Error: Bad CRC!\n");
    rx_buffer_len = 0;
    RIMESTATS_ADD(badcrc);
    RELEASE_LOCK();
    return 0;
  }

  RELEASE_LOCK();

  return len;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Set the transmission power.
 * \param power The transmission power in dB.
 */
void
cc1100_set_txpower(int8_t power)
{
  GET_LOCK();
  set_txpower(power);
  RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Get the current transmission power.
 * \return The transmission power in dB.
 */
int
cc1100_get_txpower(void)
{
  uint8_t pa_value;
  int power = 0;
  int i;

  GET_LOCK();
  pa_value = getreg(CC1100_PATABLE);
  RELEASE_LOCK();

  for(i = 0; i < sizeof(CC1100_PA_LEVELS) / sizeof(CC1100_PA_LEVELS[0]); ++i) {
    if(CC1100_PA_LEVELS[i].value == pa_value) {
      power = CC1100_PA_LEVELS[i].level;
      break;
    }
  }

  return power;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Get the current RSSI.
 * \return The RSSI value in dB.
 */
int
cc1100_rssi(void)
{
  int rssi;
  int radio_was_off = 0;

  if(locked) {
    return 0;
  }

  GET_LOCK();

  if(!receive_on) {
    radio_was_off = 1;
    cc1100_on();
  }
  BUSYWAIT_UNTIL(getreg(CC1100_PKTSTATUS) & (CC1100_PKTSTATUS_CS | CC1100_PKTSTATUS_CCA), RTIMER_SECOND / 100);

  rssi = (int)(signed char)getreg(CC1100_RSSI) / 2 - 74;

  if(radio_was_off) {
    cc1100_off();
  }
  RELEASE_LOCK();
  return rssi;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Get the current RSSI.
 * \return The RSSI value in dB.
 */
/*
static int
detected_energy(void)
{
  return cc1100_rssi();
}
*/
/*---------------------------------------------------------------------------*/
/**
 * \brief Check if the current CCA value is valid.
 * \return CCA validity status.
 * \return 1 CCA is valid.
 * \return 0 CCA is not valid (Radio was not on for long enough).
 */
int
cc1100_cca_valid(void)
{
  int valid;
  if(locked) {
    return 1;
  }
  GET_LOCK();
  valid = !!(getreg(CC1100_PKTSTATUS) & (CC1100_PKTSTATUS_CS | CC1100_PKTSTATUS_CCA));
  RELEASE_LOCK();
  return valid;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Perform a Clear-Channel Assessment (CCA) to find out if there is a
 * packet in the air or not.
 * \return The CCA result status.
 * \return 1 The channel is clear.
 * \return 0 The channel is not clear.
 */
static int
cc1100_cca(void)
{
  int cca;
  int radio_was_off = 0;

  /*
   * If the radio is locked by an underlying thread (because we are
   * being invoked through an interrupt), we preted that the coast is
   * clear (i.e., no packet is currently being transmitted by a
   * neighbor).
   */
  if(locked) {
    return 1;
  }

  GET_LOCK();
  if(!receive_on) {
    radio_was_off = 1;
    cc1100_on();
  }

  /* Make sure that the radio really got turned on. */
  if(!receive_on) {
    RELEASE_LOCK();
    return 1;
  }

  BUSYWAIT_UNTIL(getreg(CC1100_PKTSTATUS) & (CC1100_PKTSTATUS_CS | CC1100_PKTSTATUS_CCA), RTIMER_SECOND / 100);

  cca = !!(getreg(CC1100_PKTSTATUS) & CC1100_PKTSTATUS_CCA);

  if(radio_was_off) {
    cc1100_off();
  }
  RELEASE_LOCK();
  return cca;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Check if the radio driver is currently receiving a packet.
 * \return Radio status.
 * \return 1 Radio is receiving a packet.
 * \return 0 Radio is not currently receiving.
 */
int
cc1100_receiving_packet(void)
{
  return CC1100_GDO2_IS_1 && ((status() & CC1100_STATE_MASK) == CC1100_STATE_RX);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Check if the radio driver has just received a packet.
 * \return Internal buffer status.
 * \return 1 A received packet is pending.
 * \return 0 No packet is pending.
 */
static int
pending_packet(void)
{
  return !!rx_buffer_len;
}
/*---------------------------------------------------------------------------*/
/** @} */
