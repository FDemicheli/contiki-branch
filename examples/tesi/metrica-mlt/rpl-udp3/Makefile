#Makefile di rpl-udp3. X usare powertrace basta abilitare WITH_COMPOWER=1
all: udp-client udp-server
APPS=servreg-hack
CONTIKI=../../../../

WITH_UIP6=1
UIP_CONF_IPV6=1
WITH_COMPOWER=1

#X disabilitare RPL devo commentare CFLAGS+= -DUIP_CONF_IPV6_RPL
CFLAGS+= -DUIP_CONF_IPV6_RPL

## patch different cycle times
CFLAGS += -DCONTIKIMAC_CONF_CYCLE_RATE=3 -DCONTIKIMAC_CONF_MIN_CYCLE_RATE=5
##

ifdef WITH_COMPOWER
APPS+=powertrace
CFLAGS+= -DCONTIKIMAC_CONF_COMPOWER=1 -DWITH_COMPOWER=1 -DQUEUEBUF_CONF_NUM=4
endif

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif
ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif

include $(CONTIKI)/Makefile.include
