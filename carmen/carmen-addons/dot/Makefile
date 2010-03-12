include ../Makefile.conf


MODULE_NAME = DOT
MODULE_COMMENT = Dynamic Object Tracker

SOURCES = dot.c dot_interface.c dots.c contour.c scanmatch.c dots_util.c #shape.c

CFLAGS += -pg
IFLAGS 	+=
LFLAGS 	+= 	-lparam_interface -lglobal -llocalize_interface -lgeometry \
		-lipc -llaser_interface -llocalize_core \
		-lrobot_interface -lmap_interface -lgsl -lgslcblas

ifndef NO_ZLIB
LFLAGS += -lz
endif

PUBLIC_INCLUDES = dot_interface.h dot_messages.h dot.h
PUBLIC_LIBRARIES = libdot_interface.a
PUBLIC_BINARIES = dot dots

TARGETS = dot libdot_interface.a dots

ifndef NO_GRAPHICS
IFLAGS 	+= `$(GTK_CONFIG) --cflags`
LFLAGS 	+= -lglobal_graphics -lmap_graphics -lnavigator_interface -lgdk_imlib `$(GTK_CONFIG) --libs`
SOURCES += cluster.c film.c intersect.c convex_hull_test.c scan_match_test.c dots_graphics.c
TARGETS += cluster film intersect convex_hull_test scan_match_test
endif

all:

dot: dot.o

libdot_interface.a: dot_interface.o


ifndef NO_GRAPHICS
dots: dots.o dots_util.o dots_graphics.o contour.o scanmatch.o #shape.o
cluster: cluster.o
film: film.o
intersect: intersect.o
convex_hull_test: convex_hull_test.o
scan_match_test: scan_match_test.o
else
dots: dots.o dots_util.o contour.o scanmatch.o #shape.o
endif

include $(CARMEN_HOME)/src/Makefile.rules
