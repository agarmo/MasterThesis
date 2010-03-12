include ../Makefile.conf


MODULE_NAME = DOG
MODULE_COMMENT = Dynamic Occupancy Grid

SOURCES = dog.c

CFLAGS += -pg
IFLAGS 	+=
LFLAGS 	+= 	-lparam_interface -lglobal -llocalize_interface -lgeometry \
		-lipc -llaser_interface -lrobot_interface -lmap_interface

ifndef NO_ZLIB
LFLAGS += -lz
endif

PUBLIC_INCLUDES = 
PUBLIC_LIBRARIES = 
PUBLIC_BINARIES = dog

TARGETS = dog

ifndef NO_GRAPHICS
IFLAGS 	+= `$(GTK_CONFIG) --cflags`
LFLAGS 	+= -lglobal_graphics -lmap_graphics -lgdk_imlib `$(GTK_CONFIG) --libs`
SOURCES += 
TARGETS += 
endif

all:

dog: dog.o

include $(CARMEN_HOME)/src/Makefile.rules
