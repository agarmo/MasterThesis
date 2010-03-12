include ../Makefile.conf

SUBDIRS = LDLoad Carmen IPC

IFLAGS += -I$(JAVA_HOME)/include 
IFLAGS += -I$(JAVA_HOME)/include/linux 
IFLAGS += -I/usr/include/bsd
IFLAGS += -I ../ipc
LFLAGS += -L ../../lib
CFLAGS += -DREDHAT_52 -DREDHAT_6 -DREDHAT_71 
CFLAGS += -Wno-unused-parameter

ifeq ($(PROCESSOR),x86_64)
CFLAGS += -fPIC
endif

MODULE_NAME = JAVA LIBRARIES
MODULE_COMMENT = Module for interfacing java programs with Carmen!

SOURCES = ipcjava.c
PUBLIC_LIBRARIES = libipcjava.so
TARGETS = libipcjava.so Carmen.jar TestBase.class TestRobot.class

libipcjava.so: ipcjava.o
	$(ECHO) "    ---- Creating shared library (C)"
	$(SILENT) ld -o $@ -G -dy $^ -L ../ipc -lipc

JAVA_FILES := $(wildcard $(patsubst %,%/*.java,$(SUBDIRS)))


Carmen.jar: $(JAVA_FILES)
	$(MAKE) -C LDLoad
	$(MAKE) -C IPC
	$(MAKE) -C Carmen
	$(ECHO) "    ---- Creating $@"
	$(SILENT) $(JAVA_HOME)/bin/jar cf Carmen.jar Carmen/*.class IPC/*.class LDLoad/*.class libipcjava.so

%.class: %.java
	$(ECHO) "    ---- Compiling $@ (Java)"
	$(SILENT) $(JAVA_HOME)/bin/javac -classpath Carmen.jar:. -d . $<

# rules

include ../Makefile.rules


