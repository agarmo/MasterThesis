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
LFLAGS += -lglobal -lipc -lparam_interface -lserial
#LFLAGS += -lgps_interface -lparam_interface -lglobal -lipc -lbase_interface \
#	  -lcompass_interface

MODULE_NAME = "Cerebellum"
MODULE_COMMENT = "Base module for Cerebellum board"

SOURCES = cerebellum.c cerebellum_main.c cerebellum_test.c \
	cerebellum_com.c

PUBLIC_LIBRARIES = libcerebellum.a  libcerebellum.so 
PUBLIC_BINARIES = cerebellum cerebellum_test

TARGETS = cerebellum cerebellum_test

all:

libcerebellum.a: cerebellum_main.o cerebellum_com.o

libcerebellum.so: cerebellum_main.o cerebellum_com.o

cerebellum: cerebellum.o libcerebellum.a

cerebellum_test: cerebellum_test.o cerebellum_com.o

# rules

include $(CARMEN_HOME)/src/Makefile.rules
