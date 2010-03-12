include ../Makefile.conf


MODULE_NAME = STRACKER
MODULE_COMMENT = Simple Object Tracker

SOURCES = stracker.c stracker_interface.c

CFLAGS +=
IFLAGS 	+=
LFLAGS 	+= 	-lmap_interface -lparam_interface -lglobal \
		-lipc -llocalize_interface -llaser_interface

PUBLIC_INCLUDES = stracker_interface.h
PUBLIC_LIBRARIES = stracker_interface.a
PUBLIC_BINARIES = stracker

TARGETS = stracker libstracker_interface.a

all:

stracker: stracker.o

libstracker_interface.a: stracker_interface.o


include $(CARMEN_HOME)/src/Makefile.rules
