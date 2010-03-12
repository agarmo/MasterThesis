include ../Makefile.conf

MODULE_NAME = "HOKUYO"
MODULE_COMMENT = "Hokuyo PB9 IR Module"

LFLAGS += -lparam_interface -lserial -lipc -lglobal

SOURCES = hokuyo_dev.c hokuyo_ipc.c hokuyo_main.c hokuyo.c hokuyo_test.c

PUBLIC_INCLUDES = hokuyo_messages.h hokuyo_interface.h
PUBLIC_LIBRARIES = libhokuyo_interface.a libhokuyo.a
PUBLIC_BINARIES = hokuyo

TARGETS = hokuyo hokuyo_test libhokuyo_interface.a libhokuyo.a

all:

hokuyo:			hokuyo_dev.o hokuyo_ipc.o hokuyo_main.o hokuyo.o

hokuyo_test:		hokuyo_test.o libhokuyo_interface.a

libhokuyo.a:		hokuyo.o hokuyo_ipc.o hokuyo_main.o

libhokuyo_interface.a:	hokuyo_interface.o

# rules

include ../Makefile.rules
