CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -pedantic
CLIBS=-lcurl -lm
prefix=/usr/local

bld/jpw: bld/tmp/jpw.o bld/tmp/jpw-update.o bld/tmp/jpw-list.o
	gcc -o bld/jpw bld/tmp/jpw.o bld/tmp/jpw-update.o bld/tmp/jpw-list.o $(CLIBS)

bld/tmp/jpw.o: bld/tmp src/jpw.c src/jpw.h
	$(CC) $(CFLAGS) -c -o bld/tmp/jpw.o src/jpw.c $(CLIBS)

bld/tmp/jpw-update.o: bld/tmp src/jpw-update.c src/jpw-update.h
	$(CC) $(CFLAGS) -c -o bld/tmp/jpw-update.o src/jpw-update.c $(CLIBS)

bld/tmp/jpw-list.o: bld/tmp src/jpw-list.c src/jpw-list.h
	$(CC) $(CFLAGS) -c -o bld/tmp/jpw-list.o src/jpw-list.c $(CLIBS)

bld/tmp:
	mkdir -p bld/tmp

.PHONY: clean
clean:
	rm -rf bld
	rm -rf bld/tmp

.PHONY: install
install:
	mkdir -p $(prefix)/bin
	cp bld/jpw $(prefix)/bin
