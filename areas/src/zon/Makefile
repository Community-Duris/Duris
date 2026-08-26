.PHONY: all clean

all: ../../make_zon

../../make_zon: make_zon.o
	gcc -o $@ $<

make_zon.o : make_zon.c
	gcc -c -g make_zon.c

clean:
	$(RM) make_zon.o ../../make_zon
