include ../Makefile.fuzzing_common

CORD_LC_SERVER_ADDR ?= "fe80::8456:17ff:fe95:b450"
CORD_LC_SERVER_ADDR_PREFIX ?= 64
CORD_LC_SERVER_PORT ?= 4223

CFLAGS += -DSERVER_ADDR=\"$(CORD_LC_SERVER_ADDR)\"
CFLAGS += -DSERVER_ADDR_PREFIX=$(CORD_LC_SERVER_ADDR_PREFIX)
CFLAGS += -DSERVER_PORT=$(CORD_LC_SERVER_PORT)

USEMODULE += gnrc_ipv6
USEMODULE += cord_lc
USEMODULE += gcoap
USEMODULE += shell
USEMODULE += shell_commands

#USEMODULE += netdev_default
#USEMODULE += auto_init_gnrc_netif
#USEMODULE += gnrc_ipv6_default
#USEMODULE += gnrc_icmpv6_echo

include $(RIOTBASE)/Makefile.include
