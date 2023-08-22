# name of your application
APPLICATION = hello-world

USEMODULE += stdin
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6
USEMODULE += gcoap_forward_proxy

CFLAGS += -DTHREAD_STACKSIZE_MAIN=2048

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
