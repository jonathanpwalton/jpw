program = jpw
version = 20250423

objects = archive.o curl.o io.o main.o print.o pull.o

CXX = g++
CPPFLAGS = -DNDEBUG
CXXFLAGS = -std=c++17 -Os -s -Wall -Wextra -pedantic -Wno-unused-value -Wno-unused-parameter -Wno-unused-variable
LD = $(CXX)
LDFLAGS = $(CXXFLAGS)
LIBS = -lcurl -larchive

.PHONY: clean

all: $(program) droproot

$(program): $(objects)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: src/%.cc Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

droproot: src/droproot.c
	$(CC) -o $@ $<

clean:
	rm -f $(program) *.o
