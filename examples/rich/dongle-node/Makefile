CONTIKI_PROJECT = dongle-node
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

TARGET = jn5168
CFLAGS += -DDONGLE_NODE

CONTIKI_WITH_IPV6 = 1

PROJECTDIRS += .. ../tools 
PROJECT_SOURCEFILES += rich.c 

CFLAGS += -DWITH_COAP
CFLAGS += -DUIP_CONF_TCP=0
APPS = json
APPS += er-coap
APPS += rest-engine

all: $(CONTIKI_PROJECT)

CONTIKI=../../..
include $(CONTIKI)/Makefile.include
