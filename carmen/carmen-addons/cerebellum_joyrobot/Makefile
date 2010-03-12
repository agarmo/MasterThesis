ifdef CARMEN_HOME
  ifeq ($(CARMEN_HOME)/src/Makefile.vars, $(wildcard $(CARMEN_HOME)/src/Makefile.vars))
    include $(CARMEN_HOME)/src/Makefile.vars
  else
    $(warning )
    $(warning The CARMEN_HOME variable no longer points to a valid directory.)
    $(warning Please set this variable to point to a CARMEN distribution,)
    $(warning or unset it, making sure this directory is in the CARMEN )
    $(warning src directory. )
    $(error )
  endif
else
  ifeq (../Makefile.vars, $(wildcard ../Makefile.vars))
    include ../Makefile.vars
  else
    $(warning )
    $(warning The CARMEN_HOME variable is not set.)
    $(warning Please set this variable to point to a CARMEN distribution,)
    $(warning or unset it, making sure this directory is in the CARMEN )
    $(warning src directory. )
    $(warning )
    $(warning If this directory is in the CARMEN src directory, then you )
    $(warning may need to run 'configure' first.)
    $(warning )
    $(error )
  endif
endif

include $(CARMEN_HOME)/src/Makefile.conf

CFLAGS +=
IFLAGS += 
LFLAGS += -lrobot_interface -lipc -lglobal -lparam_interface -lcerebellum_interface

MODULE_NAME = JOYSTICK
MODULE_COMMENT = Cerebellum Joystick library and control modules

SOURCES = keyctrl.c
PUBLIC_INCLUDES = keyctrl.h ctrl.h
PUBLIC_LIBRARIES = libkeyctrl.a
TARGETS = libkeyctrl.a 

ifndef NO_JOYSTICK
SOURCES += joystick.c joyrobot.c joyctrl.c wingman_control.c
IFLAGS += $(JOYSTICK_INCLUDE)	
PUBLIC_INCLUDES += joystick.h joyctrl.h
PUBLIC_LIBRARIES += libjoystick.a libjoyctrl.a
PUBLIC_BINARIES += joyrobot wingman_control
TARGETS += libjoystick.a libjoyctrl.a joyrobot wingman_control
endif

all:

libjoystick.a:	joystick.o

libjoyctrl.a:	joyctrl.o

libkeyctrl.a:	keyctrl.o

joyrobot:	joyrobot.o libjoyctrl.a libkeyctrl.a

wingman_control: wingman_control.o joystick.o

man:
	cp ./joyrobot.1.gz /usr/man/man1/


include $(CARMEN_HOME)/src/Makefile.rules

