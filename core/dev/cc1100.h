/**
 * \addtogroup cc1100
 * @{
 *
 * \file
 * CC1100 radio driver header file
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

#ifndef __CC1100_H__
#define __CC1100_H__

#include "contiki.h"
#include "dev/spi.h"
#include "dev/radio.h"
#include "dev/cc1100_const.h"

int cc1100_init(void);

/**
 * Maximum length of a radio packet.
 * Variable packet length mode allows a maximum of 255 bytes per packet,
 * from which we must substract the length byte.
 * So, the maximum acceptable value for CC1100_MAX_PACKET_LEN is 254.
 *
 * A value of 127 fits 802.15.4 frames.
 */
#define CC1100_MAX_PACKET_LEN	127

int cc1100_set_channel(int channel);
int cc1100_get_channel(void);

extern int16_t cc1100_last_rssi;
extern uint8_t cc1100_last_lqi;

int cc1100_rssi(void);

extern const struct radio_driver cc1100_driver;

void cc1100_set_txpower(int8_t power);
int cc1100_get_txpower(void);

int cc1100_interrupt(void);

int cc1100_on(void);
int cc1100_off(void);

/************************************************************************/
/* Additional SPI Macros for the CC1100 */
/************************************************************************/
/* Send a strobe to the CC1100 */
#define CC1100_STROBE(s)                                \
  do {                                                  \
    CC1100_SPI_ENABLE();                                \
    SPI_WRITE(CC1100_WRITE | s);                        \
    CC1100_SPI_DISABLE();                               \
  } while (0)

/* Write to a register in the CC1100 */
#define CC1100_WRITE_REG(adr,data)                      \
  do {                                                  \
    CC1100_SPI_ENABLE();                                \
    SPI_WRITE_FAST(CC1100_WRITE | adr);                 \
    SPI_WRITE_FAST((uint8_t)(data & 0xff));             \
    SPI_WAITFORTx_ENDED();                              \
    CC1100_SPI_DISABLE();                               \
  } while(0)

/* Write to registers in burst mode */
#define CC1100_WRITE_BURST(adr,buffer,count)            \
  do {                                                  \
    uint8_t i;                                          \
    CC1100_SPI_ENABLE();                                \
    SPI_WRITE_FAST(CC1100_WRITE | CC1100_BURST | adr);  \
    for(i = 0; i < (count); i++) {                      \
      SPI_WRITE_FAST(((uint8_t *)(buffer))[i]);         \
    }                                                   \
    SPI_WAITFORTx_ENDED();                              \
    CC1100_SPI_DISABLE();                               \
  } while(0)

/* Read a register in the CC1100 */
#define CC1100_READ_REG(adr,data)                       \
  do {                                                  \
    CC1100_SPI_ENABLE();                                \
    SPI_WRITE(CC1100_READ | adr);                       \
    (void)SPI_RXBUF;                                    \
    SPI_READ(data);                                     \
    clock_delay(1);                                     \
    CC1100_SPI_DISABLE();                               \
  } while(0)

/* Read registers in burst mode */
#define CC1100_READ_BURST(adr,buffer,count)             \
  do {                                                  \
    uint8_t i;                                          \
    CC1100_SPI_ENABLE();                                \
    SPI_WRITE(CC1100_READ | CC1100_BURST | adr);        \
    (void)SPI_RXBUF;                                    \
    for(i = 0; i < (count); i++) {                      \
      SPI_READ(((uint8_t *)(buffer))[i]);               \
    }                                                   \
    clock_delay(1);                                     \
    CC1100_SPI_DISABLE();                               \
  } while(0)

/* Read form the FIFO */
#define CC1100_READ_FIFO_BYTE(data)                     \
  do {                                                  \
    CC1100_SPI_ENABLE();                                \
    SPI_WRITE(CC1100_READ | CC1100_RXFIFO);             \
    (void)SPI_RXBUF;                                    \
    SPI_READ(data);                                     \
    clock_delay(1);                                     \
    CC1100_SPI_DISABLE();                               \
  } while(0)

/* Read form the FIFO in burst mode */
#define CC1100_READ_FIFO_BUF(buffer,count)                      \
  do {                                                          \
    uint8_t i;                                                  \
    CC1100_SPI_ENABLE();                                        \
    SPI_WRITE(CC1100_READ | CC1100_BURST | CC1100_RXFIFO);      \
    (void)SPI_RXBUF;                                            \
    for(i = 0; i < (count); i++) {                              \
      SPI_READ(((uint8_t *)(buffer))[i]);                       \
    }                                                           \
    clock_delay(1);                                             \
    CC1100_SPI_DISABLE();                                       \
  } while(0)

/* Write to the FIFO in burst mode */
#define CC1100_WRITE_FIFO_BUF(buffer,count)                             \
  do {                                                                  \
    uint8_t i;                                                          \
    CC1100_SPI_ENABLE();                                                \
    SPI_WRITE_FAST(CC1100_WRITE | CC1100_BURST | CC1100_TXFIFO);        \
    for(i = 0; i < (count); i++) {                                      \
      SPI_WRITE_FAST(((uint8_t *)(buffer))[i]);                         \
    }                                                                   \
    SPI_WAITFORTx_ENDED();                                              \
    CC1100_SPI_DISABLE();                                               \
  } while(0)

/* Read status of the CC1100, with RX FIFO info */
#define CC1100_GET_STATUS(s)                    \
  do {                                          \
    CC1100_SPI_ENABLE();                        \
    SPI_WRITE(CC1100_READ | CC1100_SNOP);       \
    s = SPI_RXBUF;                              \
    CC1100_SPI_DISABLE();                       \
  } while (0)

/* Read status of the CC1100, with TX FIFO info */
#define CC1100_GET_STATUS_TX(s)                 \
  do {                                          \
    CC1100_SPI_ENABLE();                        \
    SPI_WRITE(CC1100_WRITE | CC1100_SNOP);      \
    s = SPI_RXBUF;                              \
    CC1100_SPI_DISABLE();                       \
  } while (0)

#endif /* __CC1100_H__ */
/** @} */
