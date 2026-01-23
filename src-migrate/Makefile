# makefile for migrate_pfiles tool

CC = g++
CFLAGS = -g -Wno-write-strings -std=c++14 -D_FORTIFY_SOURCE=0 -DTEST_MUD
DEBUG = -ggdb3

INCLUDES = -I. -I../src -I/usr/include/libxml2 -I/usr/include/mysql -I../src/ships
LDFLAGS = -L/usr/lib/mysql
LIBS = -lm -lstdc++ -lz -lxml2 -lcrypt -lmysqlclient -lgnutls -lssl -lcrypto -lcjson -lpthread

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

OBJS = $(SRCS:.c=.o)

TARGET = migrate_pfiles

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c migrate_common.h
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c $< -o $@

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean
