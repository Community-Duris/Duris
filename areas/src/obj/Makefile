.PHONY: all clean

all: ../../make_obj

../../make_obj: make_obj.o
	gcc -o $@ $<

make_obj.o : make_obj.c
	gcc -c -g make_obj.c

clean:
	$(RM) make_obj.o ../../make_obj
