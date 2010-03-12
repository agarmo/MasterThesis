include ../Makefile.conf

CFLAGS +=
IFLAGS += 
LFLAGS += -lrobot_interface -lipc -lglobal -lparam_interface

MODULE_NAME = JOYSTICK
MODULE_COMMENT = Joystick library and control modules

ifndef NO_JOYSTICK
SOURCES += joyctrl.c wingman_control.c
IFLAGS += $(JOYSTICK_INCLUDE)	
PUBLIC_INCLUDES += joyctrl.h
PUBLIC_LIBRARIES += libjoyctrl.a
PUBLIC_BINARIES += wingman_control
TARGETS += libjoyctrl.a wingman_control
endif

libjoyctrl.a:	joyctrl.o

# joyrobot:	libjoyctrl.a 

wingman_control: wingman_control.o joyctrl.o

# man:
#	cp ./joyrobot.1.gz /usr/man/man1/

include ../Makefile.rules
