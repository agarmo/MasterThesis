include ../Makefile.conf


MODULE_NAME = DOT
MODULE_COMMENT = Dynamic Object Tracker

SOURCES = dot.c dot_interface.c

CFLAGS +=
IFLAGS 	+=
LFLAGS 	+= 	-lparam_interface -lglobal -llocalize_interface -lgeometry \
		-lipc -llaser_interface -llocalize_core \
		-lrobot_interface -lmap_interface 

ifndef NO_ZLIB
LFLAGS += -lz
endif


PUBLIC_INCLUDES = dot_interface.h dot_messages.h dot.h
PUBLIC_LIBRARIES = libdot_interface.a
PUBLIC_BINARIES = dot

TARGETS = dot libdot_interface.a

all:

dot: dot.o

libdot_interface.a: dot_interface.o


include $(CARMEN_HOME)/src/Makefile.rules
