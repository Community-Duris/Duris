.PHONY: all clean

all: ../../make_mob

../../make_mob: make_mob.o
	gcc -o $@ $<

make_mob.o : make_mob.c
	gcc -c -g make_mob.c

clean:
	$(RM) make_mob.o ../../make_mob
