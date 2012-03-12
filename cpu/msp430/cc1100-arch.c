/**
 * \addtogroup cc1100
 * @{
 *
 * \file
 * CC1100 MSP430 dependent code
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

#include "contiki.h"
#include "contiki-net.h"

#include "dev/spi.h"
#include "dev/cc1100.h"
#include "isr_compat.h"

/*---------------------------------------------------------------------------*/
ISR(CC1100_IRQ, cc1100_port1_interrupt)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  if(cc1100_interrupt()) {
    LPM4_EXIT;
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}

/*---------------------------------------------------------------------------*/
void
cc1100_arch_init(void)
{
  spi_init();

  /* all input by default, set CSN as output */
  CC1100_CSN_PORT(DIR) |= BV(CC1100_CSN_PIN);

  CC1100_SPI_DISABLE();                /* Unselect radio. */
}
/*---------------------------------------------------------------------------*/
