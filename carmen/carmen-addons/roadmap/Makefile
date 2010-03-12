CARMEN_DEBUG = 1

include ../Makefile.vars
include $(CARMEN_HOME)/src/Makefile.conf

IFLAGS += `$(CARMEN_HOME)/bin/carmen-config --cflags --graphics navigation`
LFLAGS += `$(CARMEN_HOME)/bin/carmen-config --libs --graphics navigation` 
CFLAGS += -pg

LFLAGS += -ldot_interface -lsimulator_interface #-lefence

MODULE_NAME = Roadmap Navigator
MODULE_COMMENT = Another motion planner!

SOURCES = roadmap_test.c navigator.c navigator_ipc.c planner.c dynamics.c \
	fwd_lazy_dynamic_roadmap.c dynamic_roadmap.c refine_roadmap.c \
	mdp_roadmap.c dot_simul.c

PUBLIC_INCLUDES = 
PUBLIC_LIBRARIES = 

PUBLIC_BINARIES = 

TARGETS = roadmap_test navigator libdynamic_roadmap.a dot_simul 

all:

libdynamic_roadmap.a: dynamic_roadmap.o dynamics.o refine_roadmap.o \
	mdp_roadmap.o

libfwd_lazy_dynamic_roadmap.a: fwd_lazy_dynamic_roadmap.o dynamics.o

roadmap_test: roadmap_test.o libdynamic_roadmap.a

navigator: navigator.o navigator_ipc.o planner.o libdynamic_roadmap.a

dot_simul: dot_simul.o libdynamic_roadmap.a

# rules

include $(CARMEN_HOME)/src/Makefile.rules
