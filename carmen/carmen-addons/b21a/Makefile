include ../Makefile.conf

CFLAGS = -DREDHAT_52 -DREDHAT_6 -DUSE_IPC
IFLAGS += 
LFLAGS += -LB21 -lmspmodule -lmsp -labus -lRai -lUtils -lparam_interface -lglobal -lipc

MODULE_NAME = B21A
MODULE_COMMENT = ABUS B21 Module - Adapted from CTR by Reid Simmons

SOURCES = devUtils.c timeUtils.c handlers.c abus_sensors.c b21a_base.c b21a.c
PUBLIC_INCLUDES = 
PUBLIC_LIBRARIES = 
PUBLIC_BINARIES = 
MAN_PAGES =

TARGETS = libb21a.a b21a

# rules

all:

libb21a.a:	devUtils.o timeUtils.o handlers.o abus_sensors.o b21a_base.o

b21a:		b21a.c libb21a.a

include ../Makefile.rules
