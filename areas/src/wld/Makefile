.PHONY: all clean

all: ../../make_wld

../../make_wld: make_wld.o
	gcc -o $@ $<

make_wld.o : make_wld.c
	gcc -c -g make_wld.c

clean:
	$(RM) make_wld.o ../../make_wld
