/*All'inizio i led sono spenti. Se premo il bottone li accendo e li spengo alternativamente */

#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "lib/sensors.h"
#include "contiki.h"
#include <stdio.h> /* For printf() */

/*---------------------------------------------------------------------------*/
PROCESS(example_button, "Example with user-button");
AUTOSTART_PROCESSES(&example_button);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(example_button, ev, data)
{
  PROCESS_EXITHANDLER()
  PROCESS_BEGIN();
  
  SENSORS_ACTIVATE(button_sensor);//attiva il bottonr del sensore
  //button_sensor.configure(SENSORS_ACTIVE, 1);//il programma funziona lo stesso anche se la riga è commentatata
  
  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    printf("button pressed\n");
    leds_toggle(LEDS_ALL);//inverti la configurazione dei led
  }
  
  PROCESS_END();
}


