.PHONY: all clean

all: ../../make_shp

../../make_shp: make_shp.o
	gcc -o $@ $<

make_shp.o : make_shp.c
	gcc -c -g make_shp.c

clean:
	$(RM) make_shp.o ../../make_shp
