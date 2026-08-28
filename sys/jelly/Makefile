MODULE = jelly

SRC = jelly.c
SRC += gnrc_netif.c
ifneq (,$(filter saul_default%,$(USEMODULE)))
	SRC += saul.c
endif

include $(RIOTBASE)/Makefile.base
