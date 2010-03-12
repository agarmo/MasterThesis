include ../Makefile.conf

IFLAGS +=
LFLAGS += $(INTERFACE_LIBS) -lglobal -lipc -lbase_interface \
	-llocalize_interface -lgeometry -lmap_interface \
	-lparam_interface -lsimulator_interface -lrobot_interface 

MODULE_NAME = NAVIGATOR
MODULE_COMMENT = The motion planner!

SOURCES = navigator.c conventional.c planner.c navigator_ipc.c \
	navigator_interface.c trajectory.c map_modify.c \
	navigator_test.c random_nav.c

PUBLIC_INCLUDES = navigator_messages.h navigator_interface.h navigator.h \
	planner_interface.h
PUBLIC_LIBRARIES =  libnavigator_interface.a libconventional.a

PUBLIC_BINARIES = navigator

TARGETS =  libnavigator_interface.a navigator navigator_test random_nav libconventional.a

PUBLIC_LIBRARIES_SO = libnavigator_interface.so
ifndef NO_PYTHON
TARGETS += libnavigator_interface.so.1
endif

ifndef NO_TCPD
LFLAGS += -lwrap -lnsl
endif

ifndef NO_GRAPHICS
IFLAGS += `$(GTK_CONFIG) --cflags` 
LFLAGS += -lmap_graphics -lglobal_graphics `pkg-config --libs gtk+-2.0`

ifndef NO_LIBART
LFLAGS += -lart_lgpl -lartwrap
endif

SOURCES += navigator_graphics.c navigator_panel.c test_map_modify.c \
	test_panel_config.c 
PUBLIC_BINARIES += navigatorgui test_gui_config
TARGETS += navigatorgui test_map_modify test_gui_config
endif

navigator: navigator.o navigator_ipc.o conventional.o planner.o \
	trajectory.o map_modify.o

libnavigator_interface.a: navigator_interface.o

libnavigator_interface.so.1: navigator_interface.o

navigatorgui: navigator_panel.o navigator_graphics.o \
	libnavigator_interface.a 

libconventional.a: 	conventional.o planner.o trajectory.o map_modify.o

test_map_modify: test_map_modify.o map_modify.o

test_gui_config: test_panel_config.o

navigator_test: navigator_test.o navigator_interface.o

random_nav: random_nav.o libnavigator_interface.a

followpath: followpath.o libnavigator_interface.a

include ../Makefile.rules
