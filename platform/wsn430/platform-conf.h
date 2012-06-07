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
 * $Id: $
 */

/**
 * \file
 *         A brief description of what this file is
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

/*
 * Definitions below are dictated by the hardware and not really
 * changeable!
 */
/* Platform WSN430 */
#define WSN430 1

#define PLATFORM_HAS_LEDS    1
#define PLATFORM_HAS_BUTTON  0
#define PLATFORM_HAS_LIGHT   0
#define PLATFORM_HAS_BATTERY 1
#define PLATFORM_HAS_SHT11   0

/* CPU target speed in Hz */
#define F_CPU 3900000uL /*2457600uL*/

/* Our clock resolution, this is the same as Unix HZ. */
#define CLOCK_CONF_SECOND 128UL

#define BAUD2UBR(baud) ((F_CPU/baud))

#define CCIF
#define CLIF

#define HAVE_STDINT_H
#include "msp430def.h"


/* Types for clocks and uip_stats */
typedef unsigned short uip_stats_t;
typedef unsigned long clock_time_t;
typedef unsigned long off_t;

/* the low-level radio driver */
#if WITH_CC1100
#define NETSTACK_CONF_RADIO   cc1100_driver
#elif WITH_CC2420 /* WITH_CC1100 */
#define NETSTACK_CONF_RADIO   cc2420_driver
#endif /* WITH_CC2420 */

/* LED ports */
#define LEDS_PxDIR P5DIR
#define LEDS_PxOUT P5OUT
#define LEDS_CONF_RED    0x10
#define LEDS_CONF_GREEN  0x20
#define LEDS_CONF_YELLOW 0x40

/* DCO speed resynchronization for more robust UART, etc. */
#ifndef DCOSYNCH_CONF_ENABLED
#define DCOSYNCH_CONF_ENABLED 1
#endif /* DCOSYNCH_CONF_ENABLED */
#ifndef DCOSYNCH_CONF_PERIOD
#define DCOSYNCH_CONF_PERIOD 30
#endif /* DCOSYNCH_CONF_PERIOD */

#define ROM_ERASE_UNIT_SIZE  512
#define XMEM_ERASE_UNIT_SIZE (64*1024L)


#define CFS_CONF_OFFSET_TYPE    long


/* Use the first 64k of external flash for node configuration */
#define NODE_ID_XMEM_OFFSET     (0 * XMEM_ERASE_UNIT_SIZE)

/* Use the second 64k of external flash for codeprop. */
#define EEPROMFS_ADDR_CODEPROP  (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_XMEM_CONF_OFFSET    (2 * XMEM_ERASE_UNIT_SIZE)
#define CFS_XMEM_CONF_SIZE      (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_RAM_CONF_SIZE 4096

/*
 * SPI bus configuration for the WSN430.
 */

/* SPI input/output registers. */
#define SPI_TXBUF U1TXBUF
#define SPI_RXBUF U1RXBUF

                                /* USART1 Tx ready? */
#define SPI_WAITFOREOTx() while ((U1TCTL & TXEPT) == 0)
                                /* USART1 Rx ready? */
#define SPI_WAITFOREORx() while ((IFG2 & URXIFG1) == 0)
                                /* USART1 Tx buffer ready? */
#define SPI_WAITFORTxREADY() while ((IFG2 & UTXIFG1) == 0)

#define SCK            3  /* P5.3 - Output: SPI Serial Clock (SCLK) */
#define MOSI           1  /* P5.1 - Output: SPI Master out - slave in (MOSI) */
#define MISO           2  /* P5.2 - Input:  SPI Master in - slave out (MISO) */

/*
 * SPI bus - M25P80 external flash configuration.
 */

#define FLASH_PROT      2       /* P1.2 Output */
#define FLASH_CS        4       /* P4.4 Output */
#define FLASH_HOLD      7       /* P4.7 Output */

/* Enable/disable flash access to the SPI bus (active low). */

#define SPI_FLASH_ENABLE()  ( P4OUT &= ~BV(FLASH_CS) )
#define SPI_FLASH_DISABLE() ( P4OUT |=  BV(FLASH_CS) )

#define SPI_FLASH_HOLD()                ( P4OUT &= ~BV(FLASH_HOLD) )
#define SPI_FLASH_UNHOLD()              ( P4OUT |=  BV(FLASH_HOLD) )

#if WITH_CC1100

/*
 * SPI bus - CC1100 pin configuration.
 */

/* P1.3 - Input: GDO0 from CC1100 */
#define CC1100_GDO0_PORT(type)     P1##type
#define CC1100_GDO0_PIN            3
/* P5.2 - Input: GDO1 from CC1100 */
#define CC1100_GDO1_PORT(type)     P5##type
#define CC1100_GDO1_PIN            2
/* P1.4 - Input: GDO2 from CC1100 */
#define CC1100_GDO2_PORT(type)     P1##type
#define CC1100_GDO2_PIN            4
/* P4.2 - Output: SPI Chip Select (CS_N) */
#define CC1100_CSN_PORT(type)      P4##type
#define CC1100_CSN_PIN             2

#define CC1100_IRQ_VECTOR PORT1_VECTOR

/* Pin status. */
#define CC1100_GDO0_IS_1 (!!(CC1100_GDO0_PORT(IN) & BV(CC1100_GDO0_PIN)))
#define CC1100_GDO2_IS_1 (!!(CC1100_GDO2_PORT(IN) & BV(CC1100_GDO2_PIN)))

/* CC1100 rising edge trigger for external interrupt 0 (GDO0). */
#define CC1100_GDO0_INT_INIT() do {                  \
    CC1100_GDO0_PORT(IES) &= ~BV(CC1100_GDO0_PIN);  \
    CC1100_CLEAR_GDO0_INT();                         \
  } while(0)

/* GDO0 on external interrupt 0. */
#define CC1100_ENABLE_GDO0_INT()  do {CC1100_GDO0_PORT(IE) |= BV(CC1100_GDO0_PIN);} while(0)
#define CC1100_DISABLE_GDO0_INT() do {CC1100_GDO0_PORT(IE) &= ~BV(CC1100_GDO0_PIN);} while(0)
#define CC1100_CLEAR_GDO0_INT()   do {CC1100_GDO0_PORT(IFG) &= ~BV(CC1100_GDO0_PIN);} while(0)

/*
 * Enables/disables CC1100 access to the SPI bus (not the bus).
 * (Chip Select)
 */

 /* ENABLE CSn (active low) */
 /* Also waits for CHIP_RDYn (SO/GDO1 going low) */
#define CC1100_SPI_ENABLE()     do {CC1100_CSN_PORT(OUT) &= ~BV(CC1100_CSN_PIN); \
  while(CC1100_GDO1_PORT(IN) & BV(CC1100_GDO1_PIN));} while(0)
 /* DISABLE CSn (active low) */
#define CC1100_SPI_DISABLE()    (CC1100_CSN_PORT(OUT) |= BV(CC1100_CSN_PIN))
#define CC1100_SPI_IS_ENABLED() ((CC1100_CSN_PORT(OUT) & BV(CC1100_CSN_PIN)) != BV(CC1100_CSN_PIN))

#elif WITH_CC2420 /* WITH_CC1100 */

/*
 * SPI bus - CC2420 pin configuration.
 */

#define CC2420_CONF_SYMBOL_LOOP_COUNT 800

/* P1.0 - Input: FIFOP from CC2420 */
#define CC2420_FIFOP_PORT(type)   P1##type
#define CC2420_FIFOP_PIN          4
/* P1.3 - Input: FIFO from CC2420 */
#define CC2420_FIFO_PORT(type)     P1##type
#define CC2420_FIFO_PIN            3
/* P1.4 - Input: CCA from CC2420 */
#define CC2420_CCA_PORT(type)      P1##type
#define CC2420_CCA_PIN             6
/* P4.1 - Input:  SFD from CC2420 */
#define CC2420_SFD_PORT(type)      P1##type
#define CC2420_SFD_PIN             5
/* P4.2 - Output: SPI Chip Select (CS_N) */
#define CC2420_CSN_PORT(type)      P4##type
#define CC2420_CSN_PIN             2
/* P4.5 - Output: VREG_EN to CC2420 */
#define CC2420_VREG_PORT(type)     P3##type
#define CC2420_VREG_PIN            0
/* P4.6 - Output: RESET_N to CC2420 */
#define CC2420_RESET_PORT(type)    P1##type
#define CC2420_RESET_PIN           7

#define CC2420_IRQ_VECTOR PORT1_VECTOR

/* Pin status. */
#define CC2420_FIFOP_IS_1 (!!(CC2420_FIFOP_PORT(IN) & BV(CC2420_FIFOP_PIN)))
#define CC2420_FIFO_IS_1  (!!(CC2420_FIFO_PORT(IN) & BV(CC2420_FIFO_PIN)))
#define CC2420_CCA_IS_1   (!!(CC2420_CCA_PORT(IN) & BV(CC2420_CCA_PIN)))
#define CC2420_SFD_IS_1   (!!(CC2420_SFD_PORT(IN) & BV(CC2420_SFD_PIN)))

/* The CC2420 reset pin. */
#define SET_RESET_INACTIVE()   (CC2420_RESET_PORT(OUT) |=  BV(CC2420_RESET_PIN))
#define SET_RESET_ACTIVE()     (CC2420_RESET_PORT(OUT) &= ~BV(CC2420_RESET_PIN))

/* CC2420 voltage regulator enable pin. */
#define SET_VREG_ACTIVE()       (CC2420_VREG_PORT(OUT) |=  BV(CC2420_VREG_PIN))
#define SET_VREG_INACTIVE()     (CC2420_VREG_PORT(OUT) &= ~BV(CC2420_VREG_PIN))

/* CC2420 rising edge trigger for external interrupt 0 (FIFOP). */
#define CC2420_FIFOP_INT_INIT() do {                  \
    CC2420_FIFOP_PORT(IES) &= ~BV(CC2420_FIFOP_PIN);  \
    CC2420_CLEAR_FIFOP_INT();                         \
  } while(0)

/* FIFOP on external interrupt 0. */
#define CC2420_ENABLE_FIFOP_INT()  do {CC2420_FIFOP_PORT(IE) |= BV(CC2420_FIFOP_PIN);} while(0)
#define CC2420_DISABLE_FIFOP_INT() do {CC2420_FIFOP_PORT(IE) &= ~BV(CC2420_FIFOP_PIN);} while(0)
#define CC2420_CLEAR_FIFOP_INT()   do {CC2420_FIFOP_PORT(IFG) &= ~BV(CC2420_FIFOP_PIN);} while(0)

/*
 * Enables/disables CC2420 access to the SPI bus (not the bus).
 * (Chip Select)
 */

 /* ENABLE CSn (active low) */
#define CC2420_SPI_ENABLE()     (CC2420_CSN_PORT(OUT) &= ~BV(CC2420_CSN_PIN))
 /* DISABLE CSn (active low) */
#define CC2420_SPI_DISABLE()    (CC2420_CSN_PORT(OUT) |=  BV(CC2420_CSN_PIN))
#define CC2420_SPI_IS_ENABLED() ((CC2420_CSN_PORT(OUT) & BV(CC2420_CSN_PIN)) != BV(CC2420_CSN_PIN))

#endif /* WITH_CC2420 */

#endif /* __PLATFORM_CONF_H__ */
