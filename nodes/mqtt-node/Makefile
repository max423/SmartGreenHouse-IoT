CONTIKI_PROJECT = mqtt-sensor
all: $(CONTIKI_PROJECT)
PLATFORMS_ONLY = cc26x0-cc13x0 

PLATFORMS_EXCLUDE = sky z1 nrf52dk

CONTIKI=../../../..
SMART_GREENHOUSE=../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt

MODULES_REL += arch/platform/$(TARGET)
MODULES_REL += $(SMART_GREENHOUSE)/nodes/sensors


MAKE_NET_IPV6 = 1
MAKE_MAC ?= MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_IPV6
include $(CONTIKI)/Makefile.include
