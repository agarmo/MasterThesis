include ../Makefile.conf

NO_THETA = 1


MODULE_NAME = Sound Server
MODULE_COMMENT = Server for playback, volume control, speech synthesis, etc.

SOURCES = sound_server.c sound_server_interface.c

CFLAGS += -pg
IFLAGS 	+=
LFLAGS 	+= 	-lparam_interface -lglobal -lipc

ifndef NO_ZLIB
LFLAGS += -lz
endif

PUBLIC_INCLUDES = sound_server_messages.h sound_server_interface.h
PUBLIC_LIBRARIES = libsound_server_interface.a
PUBLIC_BINARIES = sound_server

TARGETS = sound_server libsound_server_interface.a

ifndef NO_THETA
CFLAGS += `theta-config --cflags` -DHAVE_THETA
LFLAGS += `theta-config --libs`
else
CFLAGS += -DNO_THETA
endif

all:

sound_server: sound_server.o

libsound_server_interface.a: sound_server_interface.o


include $(CARMEN_HOME)/src/Makefile.rules
