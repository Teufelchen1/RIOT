SRC = slipmux.c

ifneq (,$(filter slipmux_coap,$(USEMODULE)))
  SRC += coap.c
endif
ifneq (,$(filter slipmux_stdio,$(USEMODULE)))
  SRC += stdio.c
endif
ifneq (,$(filter slipmux_net,$(USEMODULE)))
  SRC += net.c
endif

ifneq (,$(filter slipmux_dev_init_devs,$(USEMODULE)))
  DIRS += init_devs
endif

include $(RIOTBASE)/Makefile.base
