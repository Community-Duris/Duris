# Makefile for the DE for players.  This doesn't include the "God" commands/info
#  including itemvalues and hunting for OP equipment for zone.

MALLOC_DEBUG = n
SHELL = /bin/sh

CC	= g++
DEBUG	= -ggdb3

ifdef USE_SHARED
	LDFLAGS += -rdynamic
endif

OPTS     = -O2
CXX_STANDARD = -std=c++20
CCFLAGS  = $(CXX_STANDARD) -D__UNIX__ -Wno-deprecated -D_DE_
CFLAGS   = $(CXX_STANDARD) -D__UNIX__ -D_DE_
INCLUDES = -I.
LIBS     = -lncurses -lstdc++

# files that would be compiled as dynamic libs if USE_SHARED was defined
#    make sure that they are NOT included in ANY form in OBJS!!!
# The makefile will deal with compiling SH_LIBS as normal files 
# if USE_SHARED isn't defined
SH_LIBS  =

SOURCES  = $(shell find -name "*.cpp")

ROOT_DIR          := $(abspath ../../..)
BIN_ROOT          ?= $(ROOT_DIR)/bin
EDITOR_BIN_DIR    ?= $(BIN_ROOT)/areas/editor
EDITOR_OBJ_DIR    ?= $(BIN_ROOT)/objects/areas/editor/production
DE_BINARY         ?= $(EDITOR_BIN_DIR)/de

OBJS = $(patsubst ./%.cpp,$(EDITOR_OBJ_DIR)/%.o,$(SOURCES))

# those below are .c files shared with dms
C_OBJS = $(EDITOR_OBJ_DIR)/common.o $(EDITOR_OBJ_DIR)/skills.o

#******************************************************************************#
#******************************************************************************#
#                                                                              #
# Don't mess around below this point.  Everything past here are makefile rules,#
# and NOT dependency lists that need to be changed                             #
#                                                                              #
#*******************************************************************************
#*******************************************************************************

DMS_OBJS =
PROGS = $(DE_BINARY)

.PHONY: all clean depend .depend message

ifndef USE_SHARED
	DMS_OBJS += $(SH_LIBS)
else
	PROGS += $(addprefix $(EDITOR_BIN_DIR)/,$(SH_LIBS:.o=.so))
endif

all: $(PROGS)

.PHONY: de
de: $(DE_BINARY)

$(DE_BINARY): $(OBJS) $(C_OBJS) | message
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(C_OBJS) $(LIBS)

-include $(OBJS:.o=.d) $(C_OBJS:.o=.d)

$(EDITOR_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) $(INCLUDES) $(OPTS) -MMD -MP $< -o $@

$(EDITOR_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTS) -MMD -MP -c $< -o $@

$(EDITOR_BIN_DIR)/%.so: %.cpp
	@mkdir -p $(EDITOR_OBJ_DIR) $(dir $@)
	$(CC) $(CFLAGS) -fpic -c $< -o $(EDITOR_OBJ_DIR)/$*.o
	ld -shared -o $@ /usr/lib/crtbeginS.o $(EDITOR_OBJ_DIR)/$*.o /usr/lib/crtendS.o

depend .depend:
	@/bin/echo -e "\e[4mDependencies now created during the creation of .o files.\e[0m"
	@echo $(OBJS)

clean:
	rm -f $(DE_BINARY) $(addprefix $(EDITOR_BIN_DIR)/,$(SH_LIBS:.o=.so))
	rm -rf $(EDITOR_OBJ_DIR)

message:
	@if [ ! -f $(EDITOR_OBJ_DIR)/skills.o ]; then \
	  /bin/echo -e "\e[1mCompiling skills.o may take a few seconds...\e[0m"; \
	fi

# DO NOT DELETE THIS LINE -- make depend depends on it.
