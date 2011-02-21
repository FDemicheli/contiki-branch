/**
 * \addtogroup cc1100
 * @{
 *
 * \file
 * Constants defined in the CC1100 Data Sheet
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

#ifndef CC1100_CONST_H
#define CC1100_CONST_H

/*
 * All constants are from the Chipcon CC1100 Data Sheet that at one
 * point in time could be found at
 * http://focus.ti.com/docs/prod/folders/print/cc1101.html
 *
 * The page numbers below refer to pages in this document.
 */

/* Page 28. */
enum cc1100_header_byte {
  CC1100_READ	= 0x80,
  CC1100_WRITE	= 0x00,
  CC1100_BURST	= 0x40,
};

/* Page 30. */
enum cc1100_status_byte {
  CC1100_CHIP_RDYN_MASK			= 0x80,
  CC1100_STATE_IDLE			= 0x00,
  CC1100_STATE_RX			= 0x10,
  CC1100_STATE_TX			= 0x20,
  CC1100_STATE_FSTXON			= 0x30,
  CC1100_STATE_CALIBRATE		= 0x40,
  CC1100_STATE_SETTLING			= 0x50,
  CC1100_STATE_RXFIFO_OVERFLOW		= 0x60,
  CC1100_STATE_TXFIFO_UNDERFLOW		= 0x70,
  CC1100_STATE_MASK			= 0x70,
  CC1100_FIFO_BYTES_AVAILABLE_MASK	= 0x0F,
};

enum cc1100_register {
  /* Strobes, page 65. */
  CC1100_SRES		= 0x30,
  CC1100_SFSTXON	= 0x31,
  CC1100_SXOFF		= 0x32,
  CC1100_SCAL		= 0x33,
  CC1100_SRX		= 0x34,
  CC1100_STX		= 0x35,
  CC1100_SIDLE		= 0x36,
  CC1100_SWOR		= 0x38,
  CC1100_SPWD		= 0x39,
  CC1100_SFRX		= 0x3A,
  CC1100_SFTX		= 0x3B,
  CC1100_SWORRST	= 0x3C,
  CC1100_SNOP		= 0x3D,

  /* Configuration Registers, page 66. */
  CC1100_IOCFG2		= 0x00,
  CC1100_IOCFG1		= 0x01,
  CC1100_IOCFG0		= 0x02,
  CC1100_FIFOTHR	= 0x03,
  CC1100_SYNC1		= 0x04,
  CC1100_SYNC0		= 0x05,
  CC1100_PKTLEN		= 0x06,
  CC1100_PKTCTRL1	= 0x07,
  CC1100_PKTCTRL0	= 0x08,
  CC1100_ADDR		= 0x09,
  CC1100_CHANNR		= 0x0A,
  CC1100_FSCTRL1	= 0x0B,
  CC1100_FSCTRL0	= 0x0C,
  CC1100_FREQ2		= 0x0D,
  CC1100_FREQ1		= 0x0E,
  CC1100_FREQ0		= 0x0F,
  CC1100_MDMCFG4	= 0x10,
  CC1100_MDMCFG3	= 0x11,
  CC1100_MDMCFG2	= 0x12,
  CC1100_MDMCFG1	= 0x13,
  CC1100_MDMCFG0	= 0x14,
  CC1100_DEVIATN	= 0x15,
  CC1100_MCSM2		= 0x16,
  CC1100_MCSM1		= 0x17,
  CC1100_MCSM0		= 0x18,
  CC1100_FOCCFG		= 0x19,
  CC1100_BSCFG		= 0x1A,
  CC1100_AGCTRL2	= 0x1B,
  CC1100_AGCTRL1	= 0x1C,
  CC1100_AGCTRL0	= 0x1D,
  CC1100_WOREVT1	= 0x1E,
  CC1100_WOREVT0	= 0x1F,
  CC1100_WORCTRL	= 0x20,
  CC1100_FREND1		= 0x21,
  CC1100_FREND0		= 0x22,
  CC1100_FSCAL3		= 0x23,
  CC1100_FSCAL2		= 0x24,
  CC1100_FSCAL1		= 0x25,
  CC1100_FSCAL0		= 0x26,
  CC1100_RCCTRL1	= 0x27,
  CC1100_RCCTRL0	= 0x28,
  CC1100_FSTEST		= 0x29,
  CC1100_PTEST		= 0x2A,
  CC1100_AGCTEST	= 0x2B,
  CC1100_TEST2		= 0x2C,
  CC1100_TEST1		= 0x2D,
  CC1100_TEST0		= 0x2E,

  /* Status Registers (always accessed with burst bit set to 1), page 67. */
  CC1100_PARTNUM	= 0x30 | CC1100_BURST,
  CC1100_VERSION	= 0x31 | CC1100_BURST,
  CC1100_FREQEST	= 0x32 | CC1100_BURST,
  CC1100_LQI		= 0x33 | CC1100_BURST,
  CC1100_RSSI		= 0x34 | CC1100_BURST,
  CC1100_MARCSTATE	= 0x35 | CC1100_BURST,
  CC1100_WORTIME1	= 0x36 | CC1100_BURST,
  CC1100_WORTIME0	= 0x37 | CC1100_BURST,
  CC1100_PKTSTATUS	= 0x38 | CC1100_BURST,
  CC1100_VCO_VC_DAC	= 0x39 | CC1100_BURST,
  CC1100_TXBYTES	= 0x3A | CC1100_BURST,
  CC1100_RXBYTES	= 0x3B | CC1100_BURST,
  CC1100_RCCTRL1_STATUS	= 0x3C | CC1100_BURST,
  CC1100_RCCTRL0_STATUS	= 0x3D | CC1100_BURST,

  /* Multibyte Registers, page 68. */
  CC1100_PATABLE	= 0x3E,
  CC1100_TXFIFO		= 0x3F,
  CC1100_RXFIFO		= 0x3F,
};

/* Page 60. */
enum cc1100_gdo {
  CC1100_GDO_RXFIFO_EOP	= 0x01,
  CC1100_GDO_TXFIFO    	= 0x02,
  CC1100_GDO_SYNCWORD	= 0x06,
  CC1100_GDO_CCA	= 0x09,
  CC1100_GDO_CS		= 0x0E,
};

/* Page 92. */
enum cc1100_pktstatus {
  CC1100_PKTSTATUS_CRC_OK	= 0x80,
  CC1100_PKTSTATUS_CS		= 0x40,
  CC1100_PKTSTATUS_PQT_REACHED	= 0x20,
  CC1100_PKTSTATUS_CCA		= 0x10,
  CC1100_PKTSTATUS_SFD		= 0x08,
  CC1100_PKTSTATUS_GDO2		= 0x04,
  CC1100_PKTSTATUS_GDO0		= 0x01,
};

#endif /* CC1100_CONST_H */
/** @} */
