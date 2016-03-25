#****************************************************************************
#
# Copyright (C) 2010-2014 Terry Niu (Beijing, China)
#
# Author:	TERRY-V
# Email:	cnbj8607@163.com
# Support:	http://blog.sina.com.cn/terrynotes
#
# This is a GNU make (gmake) makefile
#
#****************************************************************************

CC		:= gcc
CXX		:= g++
LD		:= g++
AR		:= ar rc
RANLIB		:= ranlib

VPATH		:= ./:../include/common:../src:../lib

DEBUG_CFLAGS	:= -Wall -Wno-strict-aliasing -Wno-unused-function -g -DDEBUG -D__multi_thread -DVERBOSE_MODE
RELEASE_CFLAGS	:= -Wall -Wno-strict-aliasing -Wno-unused-function -O3 -D__multi_thread -DVERBOSE_MODE

DEBUG_CXXFLAGS	:= ${DEBUG_CFLAGS} -fexceptions -fPIC
RELEASE_CXXFLAGS:= ${RELEASE_CFLAGS} -fexceptions -fPIC

DEBUG_LDFLAGS	:= -g -lcurl -lpthread
RELEASE_LDFLAGS	:= -lcurl -lpthread

SHARE_FLAGS 	:= -shared

ifeq (YES, ${DEBUG})
	CFLAGS	:= ${DEBUG_CFLAGS}
	CXXFLAGS:= ${DEBUG_CXXFLAGS}
	LDFLAGS	:= ${DEBUG_LDFLAGS}
else
	CFLAGS	:= ${RELEASE_CFLAGS}
	CXXFLAGS:= ${RELEASE_CXXFLAGS}
	LDFLAGS	:= ${RELEASE_LDFLAGS}
endif

#****************************************************************************
# Include paths
#****************************************************************************

INCS		:= -I../include/common
INCS		+= -I../src

#****************************************************************************
# Include libs
#****************************************************************************

LIBS		:= `pkg-config --libs --cflags opencv`

#****************************************************************************
# Targets of the build
#****************************************************************************

OUTPUT		:= ../bin/qSaver

all: ${OUTPUT}

#****************************************************************************
# Source files
#****************************************************************************

SRCS		:= qbitmap.cc
SRCS		+= qdir.cc
SRCS		+= qfile.cc
SRCS		+= qlogger.cc
SRCS		+= qnetworkaccessmanager.cc
SRCS		+= qservice.cc
SRCS		+= qsocketpool.cc
SRCS		+= qsaver.cc
SRCS		+= QSC_SAVER.cc

OBJS		:= $(addsuffix .o, $(basename ${SRCS}))

#****************************************************************************
# Output
#****************************************************************************

${OUTPUT}: $(OBJS)
	@echo Linking ...
	${LD} -o $@ ${OBJS} ${LIBS} ${LDFLAGS}
	@echo ^_^ Congratulations! ^_^

#****************************************************************************
# Common rules
#****************************************************************************

# Rules for compiling source files to object files
%.o: %.c
	@echo Compiling $< ...
	${CC} ${CFLAGS} ${INCS} -o $@ -c $<

%.o: %.cc
	@echo Compiling $< ...
	${CXX} ${CXXFLAGS} ${INCS} -o $@ -c $<

%.o: %.cpp
	@echo Compiling $< ...
	${CXX} ${CXXFLAGS} ${INCS} -o $@ -c $<

clean:
	-rm -f core *.o ${OUTPUT}

