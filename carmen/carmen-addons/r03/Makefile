include ../Makefile.conf

NO_THETA = 1


MODULE_NAME = R03
MODULE_COMMENT = Code for the NIH R03 Study with Pearl and the IMP

SOURCES = neck.c pearlguide.c neck_interface.c pearlguide_interface.c pearlguide_goto.c pearlguide_set_neck_motion.c imp_goto.c \
	imp_intro.c imp_intro_interface.c pearl_intro.c r03_imp_server.c r03_imp_server_interface.c #imp_introd.c

CFLAGS += -pg
IFLAGS 	+=
LFLAGS 	+= 	-lparam_interface -lglobal -lipc -lroomnav_interface -lnavigator_interface -llocalize_interface -lmap_interface \
		-lwalker_interface -lsound_server_interface

ifndef NO_ZLIB
LFLAGS += -lz
endif

PUBLIC_INCLUDES = imp_gui_interface.h imp_gui_messages.h
PUBLIC_LIBRARIES = libimp_gui_interface.a
PUBLIC_BINARIES = neck pearlguide pearlguide_goto pearlguide_set_neck_motion imp_goto pearl_intro r03_imp_server r03_imp_server_interface

TARGETS = neck pearlguide pearlguide_goto pearlguide_set_neck_motion imp_goto libimp_gui_interface.a pearl_intro r03_imp_server libr03_imp_server_interface.a

ifndef NO_GRAPHICS
CFLAGS +=
IFLAGS 	+=	`$(GTK_CONFIG) --cflags`
LFLAGS 	+= 	-lglobal_graphics -lgdk_imlib `$(GTK_CONFIG) --libs`
SOURCES +=	r03_imp_gui.c
PUBLIC_BINARIES +=	r03_imp_gui
TARGETS +=	r03_imp_gui
ifndef NO_THETA
SOURCES += woz.c
PUBLIC_BINARIES += woz
TARGETS += woz
endif
endif


ifndef NO_THETA
CFLAGS += `theta-config --cflags`
IFLAGS +=
LFLAGS += `theta-config --libs`
SOURCES += imp_intro.c imp_introd.c
PUBLIC_BINARIES += imp_intro imp_introd
TARGETS += imp_intro imp_introd
endif

all:

r03_imp_gui: r03_imp_gui.o libimp_gui_interface.a libr03_imp_server_interface.a

r03_imp_server: r03_imp_server.o

libr03_imp_server_interface.a: r03_imp_server_interface.o

neck: neck.o

pearlguide: pearlguide.o neck_interface.o

pearlguide_goto: pearlguide_goto.o pearlguide_interface.o

pearlguide_set_neck_motion: pearlguide_set_neck_motion.o pearlguide_interface.o

imp_goto: imp_goto.o

libimp_gui_interface.a: imp_gui_interface.o

imp_intro: imp_intro.o libimp_gui_interface.a

imp_introd: imp_introd.o libimp_gui_interface.a

pearl_intro: pearl_intro.o neck_interface.o

woz: woz.o

include $(CARMEN_HOME)/src/Makefile.rules
