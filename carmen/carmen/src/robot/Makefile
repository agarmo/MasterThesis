include ../Makefile.conf

LFLAGS += -lglobal -lgeometry -lbase_interface \
	-llaser_interface -lparam_interface -lipc 

MODULE_NAME = ROBOT_DAEMON
MODULE_COMMENT = Module integrating all robot sensors and configuration

SOURCES = robot.c robot_interface.c robot_test.c \
	robot_sonar.c robot_bumper.c robot_main.c \
	robot_laser.c 
PUBLIC_INCLUDES = robot_interface.h robot_messages.h 
PUBLIC_LIBRARIES = librobot_interface.a librobot.a 
PUBLIC_BINARIES = robot

TARGETS =  librobot.a robot librobot_interface.a robot_test

PUBLIC_LIBRARIES_SO = librobot_interface.so
ifndef NO_PYTHON
TARGETS += librobot_interface.so.1
endif

robot:	robot.o librobot.a

librobot.a: robot_sonar.o robot_bumper.o robot_main.o robot_laser.o 

librobot_interface.a:	robot_interface.o
librobot_interface.so.1: robot_interface.o

robot_test:	robot_test.o librobot_interface.a

include ../Makefile.rules
