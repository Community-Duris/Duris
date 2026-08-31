# makefile for migrate_pfiles tool

CC = g++
CFLAGS = -g -Wno-write-strings -std=c++14 -D_FORTIFY_SOURCE=0 -DTEST_MUD
DEBUG = -ggdb3

INCLUDES = -I. -I../../src -I/usr/include/libxml2 -I/usr/include/mysql -I../../src/ships
LDFLAGS = -L/usr/lib/mysql
LIBS = -lm -lstdc++ -lz -lxml2 -lcrypt -lmysqlclient -lgnutls -lssl -lcrypto -lcjson -lpthread

ROOT_DIR := $(abspath ../..)
BIN_ROOT ?= $(ROOT_DIR)/bin
MIGRATION_BIN_DIR ?= $(BIN_ROOT)/migrations
OBJ_DIR ?= $(BIN_ROOT)/objects/migrations

SRCS = migrate_main.c \
       migrate_common.c \
       migrate_objects.c \
       migrate_accounts.c \
       migrate_players.c \
       migrate_lockers.c \
       migrate_ships.c \
       migrate_guilds.c \
       migrate_misc.c \
       migrate_shopkeepers.c \
       migrate_stubs.c

OBJS = $(addprefix $(OBJ_DIR)/,$(SRCS:.c=.o))

TARGET = $(MIGRATION_BIN_DIR)/migrate_pfiles
TARGET_AFFECTS = $(MIGRATION_BIN_DIR)/migrate_locker_affects
TARGET_CONVERTER = $(MIGRATION_BIN_DIR)/pfile_converter

AFFECTS_SRCS = migrate_locker_affects.c migrate_common.c migrate_objects.c migrate_stubs.c
AFFECTS_OBJS = $(addprefix $(OBJ_DIR)/,$(AFFECTS_SRCS:.c=.o))

all: migrate_pfiles

.PHONY: all migrate_pfiles affects pfile_converter clean

migrate_pfiles: $(TARGET)
affects: $(TARGET_AFFECTS)
pfile_converter: $(TARGET_CONVERTER)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(TARGET_AFFECTS): $(AFFECTS_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_AFFECTS) $(AFFECTS_OBJS) $(LIBS)

$(TARGET_CONVERTER): pfile_converter.c
	@mkdir -p $(dir $@)
	gcc -g -Wall -Wextra -O2 -o $@ $<

$(OBJ_DIR)/%.o: %.c migrate_common.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c $< -o $@

clean:
	rm -f $(TARGET) $(TARGET_AFFECTS) $(TARGET_CONVERTER)
	rm -rf $(OBJ_DIR)
