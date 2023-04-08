# name of your application
APPLICATION = hello-world

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP = 1

USEMODULE += esp_log_startup
USEMODULE += esp_log_tagged

CFLAGS += -Wno-enum-conversion
CFLAGS += -Wno-attributes
CFLAGS += -Wno-maybe-uninitialized

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
