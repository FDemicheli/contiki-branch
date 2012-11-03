//Questo programma accende e spegne i led dei sensori premendo il bottone

#include "contiki.h"
#include "dev/leds.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include <stdio.h>

PROCESS(blink_process, "Blink Example");
AUTOSTART_PROCESSES(&blink_process);

PROCESS_THREAD(blink_process, ev, data)
{
  PROCESS_EXITHANDLER(goto exit);//specifica un'azione quando il processo esce
  PROCESS_BEGIN();
  
  printf("++++++++++++++++++++++++++++\n");
  printf("+ LESSON 1, FIRST EXERCISE +\n");
  printf("+ Blink app w/ button sensor +\n");
  printf("++++++++++++++++++++++++++++\n");
  
  SENSORS_ACTIVATE(button_sensor); //attiva il bottone del sensore
  leds_on(LEDS_ALL);//leds_on accende i led senza pigiare il bottone
  printf("+ All leds are on\n");
  printf("Press the user button to begin\n");
  
  while(1){
     
     static uint8_t push = 0; //conta il numero di volte che il bottone viene premuto
     PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event) && (data == &button_sensor));
     //sensors_event e button_sensor disp in core/lib
     if(push %2 == 0) { //se push è pari
       leds_toggle(LEDS_ALL);//inverti lo stato dei led, quindi in questo caso li spegne
       printf("[%d] TURNING OFF ALL LEDS ... [DONE]\n", push);
       push++;       
     }
     else{
       leds_toggle(LEDS_ALL);//inverti lo stato dei led, quindi in questo caso li riaccende
       printf("[%d] TURNING ON ALL LEDS ... [DONE]\n", push);
       push++;       
     }
     
     if (push == 255) //previene l'overflow
        push = 0;
  }
  
  exit:
    leds_off(LEDS_ALL); //leds_off spegne i led senza pigiare il bottone
    
  PROCESS_END();
}
