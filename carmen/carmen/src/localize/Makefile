include ../Makefile.conf

SUBDIRS = model_learn

CFLAGS +=
IFLAGS += 
LFLAGS += -lglobal -lparam_interface -llaser_interface -lmap_interface \
	  -lrobot_interface -lipc 

MODULE_NAME = LOCALIZE
MODULE_COMMENT = Markov Localization Module

SOURCES = localize.c likelihood_map.c localizecore.c localize_interface.c \
	  localize_initialize.c localize_motion.c 
PUBLIC_INCLUDES = localize_messages.h localize_interface.h localizecore.h \
	          likelihood_map.h localize_motion.h
PUBLIC_LIBRARIES = liblocalize_interface.a liblocalize_core.a liblocalize_motion.a
PUBLIC_BINARIES = localize_initialize localize 
MAN_PAGES =

TARGETS = localize liblocalize_interface.a localize_initialize \
	  liblocalize_core.a liblocalize_motion.a model_learn/model_learner

PUBLIC_LIBRARIES_SO = liblocalize_interface.so
ifndef NO_PYTHON
TARGETS += liblocalize_interface.so.1
endif

# rules

localize_initialize:	localize_initialize.o liblocalize_interface.a

liblocalize_interface.a:	localize_interface.o

liblocalize_interface.so.1:	localize_interface.o

liblocalize_core.a: localizecore.o likelihood_map.o

liblocalize_motion.a: localize_motion.o

localize:	localize.o liblocalize_core.a liblocalize_interface.a \
	liblocalize_motion.a

localizegraph:	localizegraph.o likelihood_map.o liblocalize_interface.a

model_learn/model_learner: 
	$(MAKE) -C model_learn

include ../Makefile.rules



