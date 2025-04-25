program = jpw
version = 20250423

objects = curl.o io.o main.o print.o pull.o

CXX = g++
CPPFLAGS = -DNDEBUG
CXXFLAGS = -std=c++17 -Os -s -Wall -Wextra -pedantic -Wno-unused-value -Wno-unused-parameter -Wno-unused-variable
LD = $(CXX)
LDFLAGS = $(CXXFLAGS)
LIBS = -lcurl

.PHONY: clean

$(program): $(objects)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: src/%.cc Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(program) *.o
