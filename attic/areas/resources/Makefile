# CircleMUD 3.0 Makefile
#
# This makefile should work with no changes for compiling under SunOS, Ultrix,
# NeXTSTEP, HP/UX, DG/UX, and Linux.  If you're using AIX, IRIX, some version
# of SVR4 or Solaris, uncomment the appropriate lines below.  If you're using
# some other operating system not mentioned, just type 'make' and cross your
# fingers..

#compiler name and flags
CC = gcc
# This archived pre-C99 source relies on tentative globals being coalesced.
MYFLAGS = -g -Wall -fcommon

#flags for profiling (see hacker.doc for more information)
PROFILE = 

#remove the hash marks below if compiling under AIX
#CC = cc

#remove the hash marks below if compiling under IRIX
#CC = cc -cckr -fullwarn
#MYFLAGS = -g
#LIBS = -lmalloc

#remove the hash mark below if compiling under SVR4 or Solaris, or if your
#linker complains that the socket functions such as 'connect()', 'bind()',
#etc are undefined.
#LIBS = -lsocket -lnsl

#############################################################################

CFLAGS = $(MYFLAGS) $(PROFILE)

ROOT_DIR := $(abspath ../../..)
BIN_ROOT ?= $(ROOT_DIR)/bin
TARGET_DIR ?= $(BIN_ROOT)/attic
OBJ_DIR ?= $(BIN_ROOT)/objects/attic/resources
TARGET := $(TARGET_DIR)/add_resources

ACT_SOURCES = oldconst.c actwld.c utils.c main.c random.c
ACTOBJ = $(addprefix $(OBJ_DIR)/,$(ACT_SOURCES:.c=.o))

.PHONY: all act clean

all: act
act: $(TARGET)

$(TARGET): $(ACTOBJ)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(PROFILE) $(ACTOBJ) $(LIBS)

$(OBJ_DIR)/%.o: %.c act.h
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
