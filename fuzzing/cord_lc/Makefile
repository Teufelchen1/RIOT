include ../Makefile.fuzzing_common

IPV6_STATIC_LLADDR ?= '"2001:db8::2"'
CFLAGS += -DGNRC_IPV6_STATIC_LLADDR=$(IPV6_STATIC_LLADDR)

CORD_LC_SERVER_ADDR ?= "2001:db8::1"
CORD_LC_SERVER_PORT ?= 4223

CFLAGS += -DSERVER_ADDR=\"$(CORD_LC_SERVER_ADDR)\"
CFLAGS += -DSERVER_PORT=\"$(CORD_LC_SERVER_PORT)\"

USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6
USEMODULE += gnrc_icmpv6_echo

USEMODULE += cord_lc

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += ztimer
USEMODULE += ztimer_sec

include $(RIOTBASE)/Makefile.include
