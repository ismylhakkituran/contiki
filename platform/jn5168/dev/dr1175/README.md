This directory contains the contiki driver for the sensors (light, humidity and temperature sensor) available on the 
NXP DR1175 board. This board is part of the NXP JN516x Evaluation Kit (see http://www.nxp.com/documents/leaflet/75017368.pdf). 

The dr1175 sensor code interfaces to the contiki core\lib\sensors.c framework.
The code is specificaly for the JN5168 platform, because it makes use of the platform\DK4 libraries 
of this JN5168 SDK.
examples\rich\dr1199-node.c shows an example on using this contiki driver.
