.PHONY: all clean

all: ../../make_qst

../../make_qst: make_qst.o
	gcc -o $@ $<

make_qst.o : make_qst.c
	gcc -c -g make_qst.c

clean:
	$(RM) make_qst.o ../../make_qst
