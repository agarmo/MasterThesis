include ../Makefile.conf

MODULE_NAME = CANON
MODULE_COMMENT = Canon Powershot Camera Driver for CARMEN

CFLAGS += -Wno-shadow
IFLAGS += 
LFLAGS += -lparam_interface -lglobal -lipc -lusb -ljpeg

PUBLIC_INCLUDES = canon_messages.h canon_interface.h jpegread.h
PUBLIC_LIBRARIES = libcanon_interface.a libjpegread.a
PUBLIC_BINARIES = canon_server previewer
MAN_PAGES =

ifndef NO_LIBUSB
IFLAGS += -I$(LIBUSB_INCLUDE_PATH)
SOURCES = canon.c canon_server.c canon_ipc.c capture_test.c parse-log.c
TARGETS = libcanon.a libcanon_interface.a canon_server capture_test parse-log

ifndef NO_GRAPHICS		
IFLAGS += `$(GTK_CONFIG) --cflags`
LFLAGS += `$(GTK_CONFIG) --libs` -lgdk_imlib -lglobal_graphics
SOURCES += previewer.c
TARGETS += previewer
endif

endif

# rules

all:

libcanon.a:	canon.o

libjpegread.a:	jpegread.o

libcanon_interface.a:	canon_interface.o libjpegread.a

capture_test:	capture_test.o libcanon_interface.a

previewer:	previewer.o libjpegread.a libcanon_interface.a

canon_server:	canon_server.o canon_ipc.o libcanon.a

parse-log:	parse-log.o

include $(CARMEN_HOME)/src/Makefile.rules
