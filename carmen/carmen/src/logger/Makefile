include ../Makefile.conf

ifdef FOR_XAVIER
INTERFACE_LIBS = -lxavier_interface -lparam_interface -lsimulator_interface
else
INTERFACE_LIBS = -lrobot_interface -llaser_interface -lbase_interface \
	         -lsimulator_interface -lgps_nmea_interface -larm_interface \
		 -lpantilt_interface -limu_interface
endif

LFLAGS += $(INTERFACE_LIBS) -llocalize_interface -lparam_interface \
	  -lglobal -lipc

MODULE_NAME = LOGGER
MODULE_COMMENT = Modules for logging, displaying, and playing back data.

SOURCES = logger.c playback_interface.c playback.c laserint_to_carmenlog.c \
	  writelog.c readlog.c log_timestamp_repair.c log_corrected_laser.c \
	  test_logfileread.c arm_dump_state.c logger_interface.c logger_comment.c

PUBLIC_INCLUDES = logger.h logger_messages.h playback_messages.h \
		  playback_interface.h writelog.h readlog.h logger_interface.h
PUBLIC_LIBRARIES = libplayback_interface.a  libwritelog.a libreadlog.a liblogger_interface.a
PUBLIC_BINARIES = log_carmen play_carmen play_carmen_control logtool_carmen log_carmen_comment

TARGETS = liblogger_interface.a  libwritelog.a libreadlog.a log_carmen libplayback_interface.a  play_carmen \
	  laserint_to_carmenlog log_timestamp_repair log_corrected_laser \
	  test_logfileread arm_dump_state logtool_carmen log_carmen_comment

ifndef NO_GRAPHICS
SOURCES += playback_control.c
IFLAGS += `$(GTK_CONFIG) --cflags`
LFLAGS += -lglobal_graphics `$(GTK_CONFIG) --libs`
TARGETS += play_carmen_control
endif

PUBLIC_LIBRARIES_SO = libplayback_interface.so
ifndef NO_PYTHON
TARGETS += libplayback_interface.so.1
endif

log_carmen:			logger.o  liblogger_interface.a libwritelog.a

libwritelog.a : 	writelog.o

libreadlog.a : 		readlog.o

liblogger_interface.a :   logger_interface.o

libplayback_interface.a:playback_interface.o

libplayback_interface.so.1:playback_interface.o

play_carmen:		playback.o libreadlog.a

play_carmen_control: 	playback_control.o playback_interface.o

laserint_to_carmenlog:	laserint_to_carmenlog.o libwritelog.a

log_timestamp_repair: 	log_timestamp_repair.o

log_corrected_laser: 	log_corrected_laser.o libwritelog.a

test_logfileread:	test_logfileread.o libreadlog.a

logtool_carmen:	        logtool.o libreadlog.a

log_carmen_comment:	logger_comment.o liblogger_interface.a

arm_dump_state:		arm_dump_state.o

include ../Makefile.rules
