# name of your application
APPLICATION = cborshell-example

# If no BOARD is found in the environment, use this default:
BOARD ?= native

USEMODULE += rust_riotmodules_standalone
USEMODULE += cborshell


# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

CFLAGS += '-DTHREAD_STACKSIZE_MAIN=(THREAD_STACKSIZE_DEFAULT + 2000)'


CARGO_CHANNEL ?= stable
include $(RIOTBASE)/Makefile.include
