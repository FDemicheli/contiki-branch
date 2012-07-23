/**
 * \addtogroup uip6
 * @{
 */
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
 *         The minrank-hysteresis objective function (OCP 1).
 *
 *         This implementation uses the estimated number of 
 *         transmissions (ETX) as the additive routing metric.
 *
 *         MRHOF + ETX calcola il cammino col il minor numero di trasmissioni da un nodo al root
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */


#include "net/rpl/rpl-private.h"
#include "net/neighbor-info.h"

#define DEBUG DEBUG_NONE
//#define DEBUG DEBUG_PRINT

#include "net/uip-debug.h"

static void reset(rpl_dag_t *);
static void parent_state_callback(rpl_parent_t *, int, int);
static rpl_parent_t *best_parent(rpl_parent_t *, rpl_parent_t *);
static rpl_dag_t *best_dag(rpl_dag_t *, rpl_dag_t *);
static rpl_rank_t calculate_rank(rpl_parent_t *, rpl_rank_t);
/*
 Calculates a rank value using the parent rank and a base rank.  If "parent" is NULL, the objective function selects a default increment
 that is adds to the "base_rank". Otherwise, the OF uses information known  about "parent" to select an increment to the "base_rank".
*/

static void update_metric_container(rpl_instance_t *);

rpl_of_t rpl_of_etx = {
  reset,
  parent_state_callback,
  best_parent,
  best_dag,
  calculate_rank,
  update_metric_container,
  1
};

/* Reject parents that have a higher link metric than the following. */
#define MAX_LINK_METRIC			10 /*disabilita i collegamenti con più di 10 tx sul cammino selezionato */

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST			100 /*disabilita i cammini che necessitano di più di 100 tx*/

/* PARENT_SWITCH_THRESHOLD = la differenza tra il costo del cammino attraverso il genitore preferito e il minimo costo del cammino
                             al fine di attivare la selezione di un nuovo genitore preferito
                             
 * The rank must differ more than 1/PARENT_SWITCH_THRESHOLD_DIV in order
 * to switch preferred parent.
 */
#define PARENT_SWITCH_THRESHOLD_DIV	2

typedef uint16_t rpl_path_metric_t;

static rpl_path_metric_t
calculate_path_metric(rpl_parent_t *p)
{
  if(p == NULL || (p->mc.obj.etx == 0 && p->rank > ROOT_RANK(p->dag->instance))) {
    return MAX_PATH_COST * RPL_DAG_MC_ETX_DIVISOR;
  } else {
    long etx = p->link_metric;//all'inizio vale INITIAL_LINK_METRIC
    PRINTF("link_metric in rpl-of-etx = %ld\n", etx);
    etx = (etx * RPL_DAG_MC_ETX_DIVISOR) / NEIGHBOR_INFO_ETX_DIVISOR;
    PRINTF("etx = %lu\n", etx);
    PRINTF("p->mc.obj.etx = %u\n", p->mc.obj.etx);
    PRINTF("(uint16_t) etx = %d\n", (uint16_t) etx);
    PRINTF("p->mc.obj.etx + (uint16_t) etx = %d\n", p->mc.obj.etx + (uint16_t) etx);
    return p->mc.obj.etx + (uint16_t) etx;
  }
}

static void
reset(rpl_dag_t *sag)
{
  //Resets the objective function state for a specific DAG. This function is called when doing a global repair on the DAG.
}

static void
parent_state_callback(rpl_parent_t *parent, int known, int etx)
{
  //Receives link-layer neighbor information. The parameter "known" is set either to 0 or 1. The "etx" parameter specifies the current
 //ETX(estimated transmissions) for the neighbor.
}

static rpl_rank_t
calculate_rank(rpl_parent_t *p, rpl_rank_t base_rank) 
{
  rpl_rank_t new_rank;
  rpl_rank_t rank_increase;

  if(p == NULL) {
    if(base_rank == 0) {
      return INFINITE_RANK;
    }
    rank_increase = NEIGHBOR_INFO_FIX2ETX(INITIAL_LINK_METRIC) * RPL_MIN_HOPRANKINC; //valore intero
    //rank_increase = NEIGHBOR_INFO_FIX2ETX(80) * 256 = (80/16) * 256 = 5 * 256 = 1280
    PRINTF("rank_increase (1280) = %d\n",rank_increase);
  } else {
    /* multiply first, then scale down to avoid truncation effects */
    rank_increase = NEIGHBOR_INFO_FIX2ETX(p->link_metric * p->dag->instance->min_hoprankinc); //valore intero
    //rank_increase = p->link_metric * 256 = ETX * 256
    PRINTF("rank_increase = %d\n",rank_increase);
    if(base_rank == 0) {
      base_rank = p->rank; ///base_rank è in sostanza il rank del parent
      PRINTF("base rank = %d\n", base_rank);
    }
  }

  if(INFINITE_RANK - base_rank < rank_increase) {
    /* Reached the maximum rank. */
    new_rank = INFINITE_RANK;
  } else {
   /* Calculate the rank based on the new rank information from DIO or
      stored otherwise. */
    new_rank = base_rank + rank_increase; ///parent_rank + dipende
    PRINTF("new rank = base_rank %d + rank_increase %d = %d\n",base_rank, rank_increase, new_rank);
  }

  return new_rank;
}

static rpl_dag_t *
best_dag(rpl_dag_t *d1, rpl_dag_t *d2)
{
  if(d1->grounded != d2->grounded) {
    return d1->grounded ? d1 : d2;
  }

  if(d1->preference != d2->preference) {
    return d1->preference > d2->preference ? d1 : d2;
  }

  return d1->rank < d2->rank ? d1 : d2;
}

static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2) //Compares two parents and returns the best one, according to the OF.
{
  rpl_dag_t *dag;
  rpl_path_metric_t min_diff;
  rpl_path_metric_t p1_metric;
  rpl_path_metric_t p2_metric;

  dag = p1->dag; /* Both parents must be in the same DAG. */

  min_diff = RPL_DAG_MC_ETX_DIVISOR /
             PARENT_SWITCH_THRESHOLD_DIV; //x discriminare i due rank
	     
  //PRINTF("p1 = %d\n",p1);
  //PRINTF("p2 = %d\n",p2);
  
  p1_metric = calculate_path_metric(p1);
//  PRINTF("p1_metric = %u\n",p1_metric);
  p2_metric = calculate_path_metric(p2);
 // PRINTF("p2_metric = %u\n",p2_metric);

  /* Maintain stability of the preferred parent in case of similar ranks. */
  /*Il preferred parent viene calcolato in rpl-dag.c*/
  if(p1 == dag->preferred_parent || p2 == dag->preferred_parent) {
    if(p1_metric < p2_metric + min_diff &&
       p1_metric > p2_metric - min_diff) {
     /* PRINTF("RPL: MRHOF hysteresis: %u <= %u <= %u\n",
             p2_metric - min_diff,
             p1_metric,
             p2_metric + min_diff);*/
      return dag->preferred_parent;
    }
  }

  return p1_metric < p2_metric ? p1 : p2;
}

static void
update_metric_container(rpl_instance_t *instance)
{
  //PRINTF("Sono dentro update_metric_container\n");
  rpl_path_metric_t path_metric;
  rpl_dag_t *dag;
/*#if RPL_DAG_MC == RPL_DAG_MC_ENERGY
  uint8_t type;
#endif*/

  instance->mc.flags = RPL_DAG_MC_FLAG_P;
  instance->mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
  instance->mc.prec = 0; //indica che l'oggetto metrica ha la precedenza

  dag = instance->current_dag;

  if (!dag->joined) {
    /* We should probably do something here */
    return;
  }

  if(dag->rank == ROOT_RANK(instance)) {
    path_metric = 0;
  } else {
    path_metric = calculate_path_metric(dag->preferred_parent);//viene aggiornata la metrica di cammino in base al nuovo preferred parent
  }
  
#if RPL_DAG_MC == RPL_DAG_MC_ETX /*viene def. in rpl-conf.h riga 54*/

  instance->mc.type = RPL_DAG_MC_ETX;//la metrica è ETX
  
  instance->mc.length = sizeof(instance->mc.obj.etx);
  
  instance->mc.obj.etx = path_metric;
  PRINTF("metrica aggiornata = %u\n",instance->mc.obj.etx);

/*  PRINTF("RPL: My path ETX to the root is %u.%u\n",
	instance->mc.obj.etx / RPL_DAG_MC_ETX_DIVISOR,
	(instance->mc.obj.etx % RPL_DAG_MC_ETX_DIVISOR * 100) / RPL_DAG_MC_ETX_DIVISOR);*/

#elif RPL_DAG_MC == RPL_DAG_MC_ENERGY

/*  instance->mc.type = RPL_DAG_MC_ENERGY;//la metrica è il consumo di energia
  instance->mc.length = sizeof(instance->mc.obj.energy);

  if(dag->rank == ROOT_RANK(instance)) { //viene selezionato un nodo alimentato oppure con la batteria
    type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
  } else {
    type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
  }

  instance->mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
  instance->mc.obj.energy.energy_est = path_metric;//viene stimata l'energia rimanente
*/
#else

#error "Unsupported RPL_DAG_MC configured. See rpl.h."

#endif /* RPL_DAG_MC */
}
