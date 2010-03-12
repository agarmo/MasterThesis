include ../Makefile.conf

MODULE_NAME = ERRORGRAPH
MODULE_COMMENT = Error Graphing

SOURCES = errorgraph.c

ifndef NO_GRAPHICS
ifndef NO_GSL

CFLAGS +=
IFLAGS 	+=	`$(GTK_CONFIG) --cflags`
LFLAGS 	+= 	-lmap_io -lparam_interface -lgdk_imlib -lglobal \
		-lglobal_graphics -lipc `$(GTK_CONFIG) --libs gthread` \
		-llocalize_interface -llaser_interface -lgsl -lgslcblas

PUBLIC_BINARIES = errorgraph
TARGETS = errorgraph

endif
endif

all:

errorgraph:	errorgraph.o

# rules

include $(CARMEN_HOME)/src/Makefile.rules

