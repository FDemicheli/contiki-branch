#include "contiki.h"
#include <stdio.h>

PROCESS(process_example, "Example process");
AUTOSTART_PROCESSES(&process_example);

PROCESS_THREAD(process_example, ev, data)
{
  PROCESS_BEGIN();
  
  while(1){
    PROCESS_WAIT_EVENT();
    printf("Got event number %d\n", ev);
  }
  
  PROCESS_END();
}
