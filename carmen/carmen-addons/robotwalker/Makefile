include ../Makefile.conf

#NO_GRAPHICS = 1  #dbug

MODULE_NAME = ROBOTWALKER
MODULE_COMMENT = Robotic Walker

#SOURCES = valet.c valet_interface.c valet_run.c walker.c \
#		walker_interface.c cerebellum.c cerebellum_main.c \
#		cerebellum_test.c cerebellum_com.c
SOURCES = walker.c \
		walker_interface.c cerebellum.c cerebellum_main.c \
		cerebellum_test.c cerebellum_com.c

CFLAGS +=
IFLAGS 	+=
LFLAGS 	+= 	-lmap_io -lmap_interface -lparam_interface -lglobal \
		-lipc -lroomnav_interface -lnavigator_interface \
		-llocalize_interface -llaser_interface -lserial \
		-lrobot_interface

ifndef NO_GRAPHICS

CFLAGS +=
IFLAGS 	+=	`$(GTK_CONFIG) --cflags`
LFLAGS 	+= 	-lgdk_imlib -lglobal_graphics `$(GTK_CONFIG) --libs gthread`

endif

PUBLIC_BINARIES = 
#PUBLIC_INCLUDES = walker_interface.h valet_interface.h
PUBLIC_INCLUDES = walker_interface.h walker_messages.h
#PUBLIC_LIBRARIES = libwalker_interface.a libvaletinterface.a
PUBLIC_LIBRARIES = libwalker_interface.a

#TARGETS = valet valet_run libvalet_interface.a walker \
#		libwalker_interface.a cerebellum cerebellum_test \
#		libcerebellum_interface.a
TARGETS = walker libwalker_interface.a cerebellum cerebellum_test \
		libcerebellum_interface.a

#ifndef NO_GRAPHICS
#TARGETS += walker_gui
#endif

ifdef NO_ZLIB
CFLAGS += -DNO_ZLIB
else
LFLAGS += -lz
endif


all:

walker:	walker.o cerebellum_com.o

libwalker_interface.a:	walker_interface.o cerebellum_com.o

#walker_gui:	walker_gui.o walkerserial_interface.o walker_interface.o

#valet:	valet.o

#libvalet_interface.a:	valet_interface.o

#valet_run:	valet_run.o valet_interface.o

cerebellum: cerebellum.o cerebellum_main.o cerebellum_com.o walker_interface.o

cerebellum_test: cerebellum_test.o cerebellum_com.o

libcerebellum_interface.a: cerebellum_com.o

include $(CARMEN_HOME)/src/Makefile.rules

