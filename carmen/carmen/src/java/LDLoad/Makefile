include ../../Makefile.conf

MODULE_NAME = "LDLoad"
MODULE_COMMENT = "Helper class to extract .so file from jar"

JAVA_FILES = $(wildcard *.java)
CLASS_FILES := $(JAVA_FILES:.java=.class) $(wildcard *.class)

TARGETS = ../Carmen.jar

../Carmen.jar: $(JAVA_FILES)
	$(ECHO) "    ---- Compiling LDLoad (Java)"
	$(SILENT) $(JAVA_HOME)/bin/javac -classpath .. -d .. $(JAVA_FILES)

include ../../Makefile.rules
