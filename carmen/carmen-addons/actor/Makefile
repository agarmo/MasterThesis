include ../Makefile.conf

LFLAGS += -lglobal -lipc -lparam_interface -lnavigator_interface \
          -lreadline -ltermcap

ifndef NO_ZLIB
LFLAGS += -lz
endif

MODULE_NAME = ACTOR
MODULE_COMMENT = "Simple module that uses the navigator to follow a \
	scripted trajectory, by Matt Rosencrantz"

SOURCES = actor.c

PUBLIC_BINARIES = actor

TARGETS = actor

# rules

all:

actor: actor.o

include $(CARMEN_HOME)/src/Makefile.rules
