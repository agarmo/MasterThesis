include ../Makefile.conf

MOBILITY_HOME = /home/albert/mobility/mobility

CFLAGS = -D__x86__ -Wno-error -D_REENTRANT
IFLAGS += -I$(MOBILITY_HOME)/include -I$(MOBILITY_HOME)/tools/include
LFLAGS += -L$(MOBILITY_HOME)/lib -L$(MOBILITY_HOME)/tools/lib -lmby -lidlmby -lomniDynamic2 -lomniORB2 -ltcpwrapGK -lomnithread -lpthread -lipc -lglobal


MODULE_NAME = "B21R"
MODULE_COMMENT = "Program for interfacing to B21R Mobility server"

SOURCES = b21rProxy.cpp 
PUBLIC_INCLUDES = 
PUBLIC_LIBRARIES = 
PUBLIC_BINARIES = b21rProxy
MAN_PAGES =

TARGETS = b21rProxy

# rules

all:

b21rProxy: b21rProxy.o 

include ../Makefile.rules
