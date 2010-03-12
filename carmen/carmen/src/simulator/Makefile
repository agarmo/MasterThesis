include ../Makefile.conf

CFLAGS += -DOLD_MOTION_MODEL

IFLAGS += -I../robot
LFLAGS += -lmap_interface -lparam_interface -llocalize_interface -lrobot \
	  -lbase_interface -llaser_interface -lgeometry -llocalize_motion \
	  -lglobal -lipc 

MODULE_NAME = SIMULATOR
MODULE_COMMENT = "simulates the readings of a robot on a map"

SOURCES = simulator.c objects.c simulator_simulation.c simulator_test.c \
	simulator_interface.c

PUBLIC_INCLUDES = simulator_messages.h simulator_interface.h
PUBLIC_LIBRARIES = libsimulator_interface.a 
PUBLIC_BINARIES = simulator simulator_connect_multiple
MAN_PAGES =

TARGETS = simulator simulator_test  libsimulator_interface.a simulator_connect_multiple


ifndef NO_PYTHON
PUBLIC_LIBRARIES_SO = libsimulator_interface.so
TARGETS += libsimulator_interface.so.1
endif

# rules

simulator:	simulator.o simulator_simulation.o objects.o

libsimulator_interface.a: simulator_interface.o

libsimulator_interface.so.1: simulator_interface.o

simulator_test:	simulator_test.o

simulator_connect_multiple: simulator_connect_multiple.o simulator_interface.o

tst : tst.o simulator_graphics.o simulator_simulation.o objects.o

include ../Makefile.rules
