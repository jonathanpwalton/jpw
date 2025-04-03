CXX=g++
CXXFLAGS=-std=c++20 -Wall -Wextra -pedantic

LD=$(CXX)
LDFLAGS=
LIBS=-lcurl

OBJECTS=src/jpw.o src/update.o src/list.o src/install.o
HEADERS=src/jpw.hpp

jpw: $(OBJECTS) $(HEADERS)
	$(LD) $(LDFLAGS) -o jpw $(OBJECTS) $(LIBS)

src/jpw.o: src/jpw.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o src/jpw.o src/jpw.cpp

src/update.o: src/update.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o src/update.o src/update.cpp

src/list.o: src/list.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o src/list.o src/list.cpp

src/install.o: src/install.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o src/install.o src/install.cpp

.PHONY: clean
clean:
	rm -f jpw *.o
