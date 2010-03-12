include $(CARMEN_HOME)/src/Makefile.conf

LFLAGS += -lglobal -lgeometry -lbase_interface -llaser_interface \
	-lparam_interface -lipc

MODULE_NAME = ROBOT
MODULE_COMMENT = Module integrating all robot sensors and configuration

SOURCES = robot.c robot_interface.c robot_test.c robot_laser.c robot_sonar.c \
	robot_main.c
PUBLIC_INCLUDES = robot_interface.h robot_messages.h 
PUBLIC_LIBRARIES = librobot_interface.a librobot.a 
PUBLIC_BINARIES = robot

TARGETS = librobot.a robot librobot_interface.a robot_test

all:

robot:	robot.o librobot.a

librobot.a: robot_laser.o robot_sonar.o robot_main.o

librobot_interface.a:	robot_interface.o

robot_test:	robot_test.o librobot_interface.a

include $(CARMEN_HOME)/src/Makefile.rules

