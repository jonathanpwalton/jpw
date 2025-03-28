CPP=g++
CPPFLAGS=-std=c++11 -Wall -Wextra -pedantic
CPPLIBS=-lcurl
prefix=/usr/local

bld/jpw: bld/tmp/jpw.o bld/tmp/curl.o
	$(CPP) -o bld/jpw bld/tmp/jpw.o bld/tmp/curl.o $(CPPLIBS)

bld/tmp/curl.o: bld/tmp src/curl.cpp src/curl.hpp
	$(CPP) $(CPPFLAGS) -c -o bld/tmp/curl.o src/curl.cpp

bld/tmp/jpw.o: bld/tmp src/jpw.cpp src/jpw.hpp
	$(CPP) $(CPPFLAGS) -c -o bld/tmp/jpw.o src/jpw.cpp

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
