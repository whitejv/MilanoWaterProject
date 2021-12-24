# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g

# define any directories containing header files other than /usr/include
#
INCLUDES = -I../../include -I../include -I../include -I../../../paho.mqtt.c-master/src -I../../../json-c

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = -L../lib -L../lib -L../../../paho.mqtt.c-master/build -L../../../json-c/json-c-build

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -lm -lpaho-mqtt3c -lpaho-mqtt3a -ljson-c

# define the C source files
SRCS = monitor.c

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

# define the executable file 
MAIN = ../bin/monitor

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)
	@echo  Compile Complete

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

monitor.o: /usr/include/stdio.h /usr/include/stdlib.h /usr/include/string.h
monitor.o: /usr/include/math.h /usr/include/time.h /usr/include/features.h
monitor.o: /usr/include/stdc-predef.h /usr/include/unistd.h
monitor.o: ../../../paho.mqtt.c-master/src/MQTTClient.h
monitor.o: ../../../paho.mqtt.c-master/src/MQTTProperties.h
monitor.o: ../../../paho.mqtt.c-master/src/MQTTReasonCodes.h
monitor.o: ../../../paho.mqtt.c-master/src/MQTTSubscribeOpts.h
monitor.o: ../../../paho.mqtt.c-master/src/MQTTClientPersistence.h
