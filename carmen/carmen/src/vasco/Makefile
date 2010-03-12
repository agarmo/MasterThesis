include ../Makefile.conf

LFLAGS +=  -lreadlog -lwritelog -lparam_interface -lglobal -lipc 

ifndef NO_GRAPHICS
CFLAGS 	+= 	-Wno-shadow
IFLAGS 	+= 	`$(GTK_CONFIG) --cflags gthread-2.0`
LFLAGS 	+= 	-lmap_io -lparam_interface -llaser_interface \
		-lglobal -lglobal_graphics -lipc -lrobot_interface \
		`$(GTK_CONFIG) --libs gthread-2.0`
endif

MODULE_NAME 	= 	VASCO
MODULE_COMMENT 	= 	Scan-matching mapper by Dirk Haehnel

ifndef NO_GRAPHICS
SOURCES = vasco.c vscanmatch.c vegrid.c laserscans.c history.c gui.c tools.c \
		vascocore_utils.c vascocore_init.c vascocore_matching.c \
		vascocore_mapping.c vascocore_scan.c
PUBLIC_INCLUDES = vascocore.h egrid.h
PUBLIC_LIBRARIES = libvascocore.a libegrid.a
PUBLIC_BINARIES = vasco vasco-tiny
MAN_PAGES = vasco_help.txt
TARGETS = vasco libvascocore.a libegrid.a vasco-tiny
endif

# rules

libegrid.a:	egrid.o

vasco:		libvascocore.a vscanmatch.o vegrid.o laserscans.o \
		history.o vasco.o gui.o tools.o libegrid.a

libvascocore.a:	vascocore_utils.o vascocore_init.o vascocore_matching.o \
		vascocore_mapping.o vascocore_scan.o

#libutils.a:	mdalloc.o utils.o correction.o

vasco-tiny:	vasco-tiny.o libvascocore.a

include ../Makefile.rules
