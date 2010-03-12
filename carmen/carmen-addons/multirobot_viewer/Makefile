include $(CARMEN_HOME)/src/Makefile.conf

CFLAGS +=
IFLAGS += 
LFLAGS += -lsimulator_interface -lmap_interface -lnavigator_interface -llocalize_interface -lparam_interface -lglobal -lipc -lz

MODULE_NAME = MULTIROBOT_VIEWER
MODULE_COMMENT = Interface for viewing and controlling multiple robots

SOURCES = 
PUBLIC_INCLUDES = 
PUBLIC_LIBRARIES = 
PUBLIC_BINARIES = multirobot_viewer
MAN_PAGES =

TARGETS = 

ifndef NO_GRAPHICS
IFLAGS += `$(GTK_CONFIG) --cflags`
LFLAGS += `$(GTK_CONFIG) --libs` -lXmu -lglut -lGL -lGLU
SOURCES += multirobot_viewer.c
TARGETS += multirobot_viewer
endif

# rules

all:

multirobot_viewer:	multirobot_viewer.o

include $(CARMEN_HOME)/src/Makefile.rules
