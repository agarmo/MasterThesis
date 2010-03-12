include ../Makefile.conf

CFLAGS += 
IFLAGS +=
LFLAGS += -lmap_io -lmap_interface -lparam_interface -lglobal -lipc -llocalize_interface -lsimulator_interface

ifdef NO_ZLIB
CFLAGS += -DNO_ZLIB
else
LFLAGS += -lz
endif

MODULE_NAME = HMAP
MODULE_COMMENT = "mapserver add-on for heirarchical maps (hmaps)."

SOURCES = create_hmap.c hmap.c
PUBLIC_INCLUDES = 
PUBLIC_LIBRARIES = 
PUBLIC_BINARIES = create_hmap hmap

TARGETS = create_hmap hmap

NO_GRAPHICS = 1
ifndef NO_GRAPHICS
CFLAGS +=
IFLAGS 	+=	`$(GTK_CONFIG) --cflags`
LFLAGS 	+= 	-lmap_graphics -lglobal_graphics `$(GTK_CONFIG) --libs gthread`
SOURCES +=	hmap_editor.c
PUBLIC_BINARIES +=	hmap_editor
TARGETS +=	hmap_editor
endif

# rules

all:

create_hmap: create_hmap.o

hmap: hmap.o

ifndef NO_GRAPHICS
hmap_editor: hmap_editor.o
endif

include $(CARMEN_HOME)/src/Makefile.rules
