/**
 * \addtogroup cc1100
 * @{
 *
 * \file
 * Configuration register value sets for the CC1100
 *
 * The Configuration register values have been obtained using SmartRF Studio 7 v1.3.1
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

#ifndef __CC1100_CONF_H__
#define __CC1100_CONF_H__

/*
 * The values in CC1100_CONFIG_REGISTERS come from SmartRF Studio 7 v1.3.1
 * The only changes from the proposed ones are WHITE_DATA set to 1,
 * FS_AUTOCAL set to 3, RXOFF_MODE set to 3 (Stay in RX),
 * GDO0_CFG set to 1, GD2_CFG set to 6.
 *
 * The C1100_PA_TABLE is used to initialize the PA_TABLE, which is particularly
 * important for ASK, OOK, and PA ramping.
 *
 * When NOT doing ASK, OOK, or PA ramping, the PA level can be adjusted using
 * values in CC1100_PA_LEVELS. These values also come from SmartRF,
 * and consist of tuples of the form (power level in dB, PA register value).
 * The CC1100_PA_LEVELS list should be sorted by decreading power level.
 */

/**
 * The modulation scheme used by the radio.
 * Accepted values are GFSK, 2FSK, 4FSK, ASK, OOK, MSK.
 */
#ifndef CC1100_CONF_MODULATION
#define CC1100_CONF_MODULATION GFSK
#endif /* CC1100_CONF_MODULATION */

/**
 * The frequency band used by the radio.
 * Accepted values are 315, 433, 868, 915.
 */
#ifndef CC1100_CONF_FREQ_MHZ
#define CC1100_CONF_FREQ_MHZ 915
#endif /* CC1100_CONF_FREQ_MHZ */

/**
 * The frequency of the crystal used by the radio.
 * Accepted values are 26, 27.
 */
#ifndef CC1100_CONF_XTAL_MHZ
#define CC1100_CONF_XTAL_MHZ 26
#endif /* CC1100_CONF_XTAL_MHZ */

struct cc1100_pa_level {
  int8_t level; /* Level in dB. */
  uint8_t value; /* Register value. */
};

#if CC1100_CONF_MODULATION == GFSK && CC1100_CONF_FREQ_MHZ == 868 && CC1100_CONF_XTAL_MHZ == 26
/*
 * GSFK modulation, 868 MHz band, with a 26 MHz crystal
 * Deviation = 126.953125 
 * Base frequency = 867.999939 
 * Carrier frequency = 867.999939 
 * Channel number = 0 
 * Carrier frequency = 867.999939 
 * Modulated = true 
 * Modulation format = GFSK 
 * Manchester enable = false 
 * Sync word qualifier mode = 30/32 sync word bits detected 
 * Preamble count = 4 
 * Channel spacing = 199.951172 
 * Carrier frequency = 867.999939 
 * Data rate = 249.939 
 * RX filter BW = 541.666667 
 * Data format = Normal mode 
 * Length config = Variable packet length mode. Packet length configured by the first byte after sync word 
 * CRC enable = true 
 * Packet length = 255 
 * Device address = 0 
 * Address config = No address check 
 * CRC autoflush = false 
 * PA ramping = false 
 * TX power = 10 
 */
uint8_t CC1100_CONFIG_REGISTERS[] = {
  0x06,	/* IOCFG2	GDO2 Output Pin Configuration */
  0x2E, /* IOCFG1	GDO1 Output Pin Configuration */
  0x01, /* IOCFG0	GDO0 Output Pin Configuration */
  0x07, /* FIFOTHR	RX FIFO and TX FIFO Thresholds */
  0xD3, /* SYNC1	Sync Word, High Byte */
  0x91, /* SYNC0	Sync Word, Low Byte */
  0xFF, /* PKTLEN	Packet Length */
  0x04, /* PKTCTRL1	Packet Automation Control */
  0x45, /* PKTCTRL0	Packet Automation Control */
  0x00, /* ADDR		Device Address */
  0x00, /* CHANNR	Channel Number */
  0x0C, /* FSCTRL1	Frequency Synthesizer Control */
  0x00, /* FSCTRL0	Frequency Synthesizer Control */
  0x21, /* FREQ2	Frequency Control Word, High Byte */
  0x62, /* FREQ1	Frequency Control Word, Middle Byte */
  0x76, /* FREQ0	Frequency Control Word, Low Byte */
  0x2D, /* MDMCFG4	Modem Configuration */
  0x3B, /* MDMCFG3	Modem Configuration */
  0x13, /* MDMCFG2	Modem Configuration */
  0x22, /* MDMCFG1	Modem Configuration */
  0xF8, /* MDMCFG0	Modem Configuration */
  0x62, /* DEVIATN	Modem Deviation Setting */
  0x07, /* MCSM2	Main Radio Control State Machine Configuration */
  0x3C, /* MCSM1	Main Radio Control State Machine Configuration */
  0x38, /* MCSM0	Main Radio Control State Machine Configuration */
  0x1D, /* FOCCFG	Frequency Offset Compensation Configuration */
  0x1C, /* BSCFG	Bit Synchronization Configuration */
  0xC7, /* AGCCTRL2	AGC Control */
  0x00, /* AGCCTRL1	AGC Control */
  0xB0, /* AGCCTRL0	AGC Control */
  0x87, /* WOREVT1	High Byte Event0 Timeout */
  0x6B, /* WOREVT0	Low Byte Event0 Timeout */
  0xFB, /* WORCTRL	Wake On Radio Control */
  0xB6, /* FREND1	Front End RX Configuration */
  0x10, /* FREND0	Front End TX Configuration */
  0xEA, /* FSCAL3	Frequency Synthesizer Calibration */
  0x2A, /* FSCAL2	Frequency Synthesizer Calibration */
  0x00, /* FSCAL1	Frequency Synthesizer Calibration */
  0x1F, /* FSCAL0	Frequency Synthesizer Calibration */
  0x41, /* RCCTRL1	RC Oscillator Configuration */
  0x00, /* RCCTRL0	RC Oscillator Configuration */
  0x59, /* FSTEST	Frequency Synthesizer Calibration Control */
  0x7F, /* PTEST	Production Test */
  0x3F, /* AGCTEST	AGC Test */
  0x88, /* TEST2	Various Test Settings */
  0x31, /* TEST1	Various Test Settings */
  0x09, /* TEST0	Various Test Settings */
};

uint8_t CC1100_PA_TABLE[] = { 0xc5, };

struct cc1100_pa_level CC1100_PA_LEVELS[] = {
  {  12, 0xc0 },
  {  10, 0xc5 },
  {   7, 0xcd },
  {   5, 0x86 },
  {   0, 0x50 },
  {  -6, 0x37 },
  { -10, 0x26 },
  { -15, 0x1d },
  { -20, 0x17 },
  { -30, 0x03 },
};

#elif CC1100_CONF_MODULATION == GFSK && CC1100_CONF_FREQ_MHZ == 868 && CC1100_CONF_XTAL_MHZ == 27
/*
 * GSFK modulation, 868 MHz band, with a 27 MHz crystal
 * Deviation = 131.835938 
 * Base frequency = 867.999985 
 * Carrier frequency = 867.999985 
 * Channel number = 0 
 * Carrier frequency = 867.999985 
 * Modulated = true 
 * Modulation format = GFSK 
 * Manchester enable = false 
 * Sync word qualifier mode = 30/32 sync word bits detected 
 * Preamble count = 4 
 * Channel spacing = 199.813843 
 * Carrier frequency = 867.999985 
 * Data rate = 249.664 
 * RX filter BW = 562.500000 
 * Data format = Normal mode 
 * Length config = Variable packet length mode. Packet length configured by the first byte after sync word 
 * CRC enable = true 
 * Packet length = 255 
 * Device address = 0 
 * Address config = No address check 
 * CRC autoflush = false 
 * PA ramping = false 
 * TX power = 10
 */
uint8_t CC1100_CONFIG_REGISTERS[] = {
  0x06, /* IOCFG2	GDO2 Output Pin Configuration */
  0x2E, /* IOCFG1	GDO1 Output Pin Configuration */
  0x01, /* IOCFG0	GDO0 Output Pin Configuration */
  0x07, /* FIFOTHR	RX FIFO and TX FIFO Thresholds */
  0xD3, /* SYNC1	Sync Word, High Byte */
  0x91, /* SYNC0	Sync Word, Low Byte */
  0xFF, /* PKTLEN	Packet Length */
  0x04, /* PKTCTRL1	Packet Automation Control */
  0x45, /* PKTCTRL0	Packet Automation Control */
  0x00, /* ADDR		Device Address */
  0x00, /* CHANNR	Channel Number */
  0x0C, /* FSCTRL1	Frequency Synthesizer Control */
  0x00, /* FSCTRL0	Frequency Synthesizer Control */
  0x20, /* FREQ2	Frequency Control Word, High Byte */
  0x25, /* FREQ1	Frequency Control Word, Middle Byte */
  0xED, /* FREQ0	Frequency Control Word, Low Byte */
  0x2D, /* MDMCFG4	Modem Configuration */
  0x2F, /* MDMCFG3	Modem Configuration */
  0x13, /* MDMCFG2	Modem Configuration */
  0x22, /* MDMCFG1	Modem Configuration */
  0xE5, /* MDMCFG0	Modem Configuration */
  0x62, /* DEVIATN	Modem Deviation Setting */
  0x07, /* MCSM2	Main Radio Control State Machine Configuration */
  0x3C, /* MCSM1	Main Radio Control State Machine Configuration */
  0x38, /* MCSM0	Main Radio Control State Machine Configuration */
  0x1D, /* FOCCFG	Frequency Offset Compensation Configuration */
  0x1C, /* BSCFG	Bit Synchronization Configuration */
  0xC7, /* AGCCTRL2	AGC Control */
  0x00, /* AGCCTRL1	AGC Control */
  0xB0, /* AGCCTRL0	AGC Control */
  0x87, /* WOREVT1	High Byte Event0 Timeout */
  0x6B, /* WOREVT0	Low Byte Event0 Timeout */
  0xFB, /* WORCTRL	Wake On Radio Control */
  0xB6, /* FREND1	Front End RX Configuration */
  0x10, /* FREND0	Front End TX Configuration */
  0xEA, /* FSCAL3	Frequency Synthesizer Calibration */
  0x2A, /* FSCAL2	Frequency Synthesizer Calibration */
  0x00, /* FSCAL1	Frequency Synthesizer Calibration */
  0x1F, /* FSCAL0	Frequency Synthesizer Calibration */
  0x41, /* RCCTRL1	RC Oscillator Configuration */
  0x00, /* RCCTRL0	RC Oscillator Configuration */
  0x59, /* FSTEST	Frequency Synthesizer Calibration Control */
  0x7F, /* PTEST	Production Test */
  0x3F, /* AGCTEST	AGC Test */
  0x88, /* TEST2	Various Test Settings */
  0x31, /* TEST1	Various Test Settings */
  0x09, /* TEST0	Various Test Settings */
};

uint8_t CC1100_PA_TABLE[] = { 0xc5, };

struct cc1100_pa_level CC1100_PA_LEVELS[] = {
  {  12, 0xc0 },
  {  10, 0xc5 },
  {   7, 0xcd },
  {   5, 0x86 },
  {   0, 0x50 },
  {  -6, 0x37 },
  { -10, 0x26 },
  { -15, 0x1d },
  { -20, 0x17 },
  { -30, 0x03 },
};

#elif CC1100_CONF_MODULATION == GFSK && CC1100_CONF_FREQ_MHZ == 915 && CC1100_CONF_XTAL_MHZ == 26
/*
 * GSFK modulation, 915 MHz band, with a 26 MHz crystal
 * Deviation = 126.953125 
 * Base frequency = 901.999969 
 * Carrier frequency = 905.998993 
 * Channel number = 20 
 * Carrier frequency = 905.998993 
 * Modulated = true 
 * Modulation format = GFSK 
 * Manchester enable = false 
 * Sync word qualifier mode = 30/32 sync word bits detected 
 * Preamble count = 4 
 * Channel spacing = 199.951172 
 * Carrier frequency = 905.998993 
 * Data rate = 249.939 
 * RX filter BW = 541.666667 
 * Data format = Normal mode 
 * Length config = Variable packet length mode. Packet length configured by the first byte after sync word 
 * CRC enable = true 
 * Packet length = 255 
 * Device address = 0 
 * Address config = No address check 
 * CRC autoflush = false 
 * PA ramping = false 
 * TX power = 10 
 */
uint8_t CC1100_CONFIG_REGISTERS[] = {
  0x06, /* IOCFG2	GDO2 Output Pin Configuration */
  0x2E, /* IOCFG1	GDO1 Output Pin Configuration */
  0x01, /* IOCFG0	GDO0 Output Pin Configuration */
  0x07, /* FIFOTHR	RX FIFO and TX FIFO Thresholds */
  0xD3, /* SYNC1	Sync Word, High Byte */
  0x91, /* SYNC0	Sync Word, Low Byte */
  0xFF, /* PKTLEN	Packet Length */
  0x04, /* PKTCTRL1	Packet Automation Control */
  0x45, /* PKTCTRL0	Packet Automation Control */
  0x00, /* ADDR		Device Address */
  0x14, /* CHANNR	Channel Number */
  0x0C, /* FSCTRL1	Frequency Synthesizer Control */
  0x00, /* FSCTRL0	Frequency Synthesizer Control */
  0x22, /* FREQ2	Frequency Control Word, High Byte */
  0xB1, /* FREQ1	Frequency Control Word, Middle Byte */
  0x3B, /* FREQ0	Frequency Control Word, Low Byte */
  0x2D, /* MDMCFG4	Modem Configuration */
  0x3B, /* MDMCFG3	Modem Configuration */
  0x13, /* MDMCFG2	Modem Configuration */
  0x22, /* MDMCFG1	Modem Configuration */
  0xF8, /* MDMCFG0	Modem Configuration */
  0x62, /* DEVIATN	Modem Deviation Setting */
  0x07, /* MCSM2	Main Radio Control State Machine Configuration */
  0x3C, /* MCSM1	Main Radio Control State Machine Configuration */
  0x38, /* MCSM0	Main Radio Control State Machine Configuration */
  0x1D, /* FOCCFG	Frequency Offset Compensation Configuration */
  0x1C, /* BSCFG	Bit Synchronization Configuration */
  0xC7, /* AGCCTRL2	AGC Control */
  0x00, /* AGCCTRL1	AGC Control */
  0xB0, /* AGCCTRL0	AGC Control */
  0x87, /* WOREVT1	High Byte Event0 Timeout */
  0x6B, /* WOREVT0	Low Byte Event0 Timeout */
  0xFB, /* WORCTRL	Wake On Radio Control */
  0xB6, /* FREND1	Front End RX Configuration */
  0x10, /* FREND0	Front End TX Configuration */
  0xEA, /* FSCAL3	Frequency Synthesizer Calibration */
  0x2A, /* FSCAL2	Frequency Synthesizer Calibration */
  0x00, /* FSCAL1	Frequency Synthesizer Calibration */
  0x1F, /* FSCAL0	Frequency Synthesizer Calibration */
  0x41, /* RCCTRL1	RC Oscillator Configuration */
  0x00, /* RCCTRL0	RC Oscillator Configuration */
  0x59, /* FSTEST	Frequency Synthesizer Calibration Control */
  0x7F, /* PTEST	Production Test */
  0x3F, /* AGCTEST	AGC Test */
  0x88, /* TEST2	Various Test Settings */
  0x31, /* TEST1	Various Test Settings */
  0x09, /* TEST0	Various Test Settings */
};

uint8_t CC1100_PA_TABLE[] = { 0xc3, };

struct cc1100_pa_level CC1100_PA_LEVELS[] = {
  {  11, 0xc0 },
  {  10, 0xc3 },
  {   7, 0xcc },
  {   5, 0x84 },
  {   0, 0x8e },
  {  -6, 0x38 },
  { -10, 0x27 },
  { -15, 0x1e },
  { -20, 0x0e },
  { -30, 0x03 },
};

#elif CC1100_CONF_MODULATION == GFSK && CC1100_CONF_FREQ_MHZ == 915 && CC1100_CONF_XTAL_MHZ == 27
/*
 * GSFK modulation, 915 MHz band, with a 27 MHz crystal
 * Deviation = 131.835938 
 * Base frequency = 901.999649 
 * Carrier frequency = 905.995926 
 * Channel number = 20 
 * Carrier frequency = 905.995926 
 * Modulated = true 
 * Modulation format = GFSK 
 * Manchester enable = false 
 * Sync word qualifier mode = 30/32 sync word bits detected 
 * Preamble count = 4 
 * Channel spacing = 199.813843 
 * Carrier frequency = 905.995926 
 * Data rate = 249.664 
 * RX filter BW = 562.500000 
 * Data format = Normal mode 
 * Length config = Variable packet length mode. Packet length configured by the first byte after sync word 
 * CRC enable = true 
 * Packet length = 255 
 * Device address = 0 
 * Address config = No address check 
 * CRC autoflush = false 
 * PA ramping = false 
 * TX power = 10 
 */
uint8_t CC1100_CONFIG_REGISTERS[] = {
  0x06, /* IOCFG2	GDO2 Output Pin Configuration */
  0x2E, /* IOCFG1	GDO1 Output Pin Configuration */
  0x01, /* IOCFG0	GDO0 Output Pin Configuration */
  0x07, /* FIFOTHR	RX FIFO and TX FIFO Thresholds */
  0xD3, /* SYNC1	Sync Word, High Byte */
  0x91, /* SYNC0	Sync Word, Low Byte */
  0xFF, /* PKTLEN	Packet Length */
  0x04, /* PKTCTRL1	Packet Automation Control */
  0x45, /* PKTCTRL0	Packet Automation Control */
  0x00, /* ADDR		Device Address */
  0x14, /* CHANNR	Channel Number */
  0x0C, /* FSCTRL1	Frequency Synthesizer Control */
  0x00, /* FSCTRL0	Frequency Synthesizer Control */
  0x21, /* FREQ2	Frequency Control Word, High Byte */
  0x68, /* FREQ1	Frequency Control Word, Middle Byte */
  0x4B, /* FREQ0	Frequency Control Word, Low Byte */
  0x2D, /* MDMCFG4	Modem Configuration */
  0x2F, /* MDMCFG3	Modem Configuration */
  0x13, /* MDMCFG2	Modem Configuration */
  0x22, /* MDMCFG1	Modem Configuration */
  0xE5, /* MDMCFG0	Modem Configuration */
  0x62, /* DEVIATN	Modem Deviation Setting */
  0x07, /* MCSM2	Main Radio Control State Machine Configuration */
  0x3C, /* MCSM1	Main Radio Control State Machine Configuration */
  0x38, /* MCSM0	Main Radio Control State Machine Configuration */
  0x1D, /* FOCCFG	Frequency Offset Compensation Configuration */
  0x1C, /* BSCFG	Bit Synchronization Configuration */
  0xC7, /* AGCCTRL2	AGC Control */
  0x00, /* AGCCTRL1	AGC Control */
  0xB0, /* AGCCTRL0	AGC Control */
  0x87, /* WOREVT1	High Byte Event0 Timeout */
  0x6B, /* WOREVT0	Low Byte Event0 Timeout */
  0xFB, /* WORCTRL	Wake On Radio Control */
  0xB6, /* FREND1	Front End RX Configuration */
  0x10, /* FREND0	Front End TX Configuration */
  0xEA, /* FSCAL3	Frequency Synthesizer Calibration */
  0x2A, /* FSCAL2	Frequency Synthesizer Calibration */
  0x00, /* FSCAL1	Frequency Synthesizer Calibration */
  0x1F, /* FSCAL0	Frequency Synthesizer Calibration */
  0x41, /* RCCTRL1	RC Oscillator Configuration */
  0x00, /* RCCTRL0	RC Oscillator Configuration */
  0x59, /* FSTEST	Frequency Synthesizer Calibration Control */
  0x7F, /* PTEST	Production Test */
  0x3F, /* AGCTEST	AGC Test */
  0x88, /* TEST2	Various Test Settings */
  0x31, /* TEST1	Various Test Settings */
  0x09, /* TEST0	Various Test Settings */
};

uint8_t CC1100_PA_TABLE[] = { 0xc3, };

struct cc1100_pa_level CC1100_PA_LEVELS[] = {
  {  11, 0xc0 },
  {  10, 0xc3 },
  {   7, 0xcc },
  {   5, 0x84 },
  {   0, 0x8e },
  {  -6, 0x38 },
  { -10, 0x27 },
  { -15, 0x1e },
  { -20, 0x0e },
  { -30, 0x03 },
};

#else /* CC1100_CONFIG_REGISTERS, CC1100_PA_TABLE, CC1100_PA_LEVELS */
#error "No configuration register table defined for this (modulation, frequency, crystal freq) tuple."
#endif /* CC1100_CONFIG_REGISTERS, CC1100_PA_TABLE, CC1100_PA_LEVELS */

#endif /* __CC1100_CONF_H__ */
/** @} */
