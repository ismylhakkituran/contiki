CONTIKI_PROJECT = dr1175-node
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

TARGET = jn5168
CFLAGS += -DSENSOR_BOARD_DR1175

CONTIKI_WITH_IPV6 = 1

PROJECTDIRS += .. ../tools ../../../platform/jn5168/dev/dr1175
PROJECT_SOURCEFILES += rich.c 
PROJECT_SOURCEFILES += light-sensor.c ht-sensor.c

CFLAGS += -DWITH_COAP
CFLAGS += -DUIP_CONF_TCP=0
APPS = json
APPS += er-coap
APPS += rest-engine

all: $(CONTIKI_PROJECT)

CONTIKI=../../..
include $(CONTIKI)/Makefile.include
