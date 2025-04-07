CXX=g++
CXXFLAGS=-std=c++20 -Wall -Wextra -pedantic
LD=$(CXX)
LDFLAGS=
LIBS=-lcurl

SOURCE=src
BUILD=.build

OUTPUT=jpw
SOURCES=$(wildcard $(SOURCE)/*.cpp)
OBJECTS=$(patsubst $(SOURCE)/%.cpp,$(BUILD)/%.o,$(SOURCES))
PREFIX=/usr/local

all: dir $(BUILD)/$(OUTPUT)

dir:
	mkdir -p $(BUILD)

$(BUILD)/$(OUTPUT): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

$(OBJECTS): $(BUILD)/%.o : $(SOURCE)/%.cpp $(SOURCE)/core.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

install:
	cp $(BUILD)/$(OUTPUT) $(PREFIX)/bin/$(OUTPUT)

clean:
	rm -f $(BUILD)/*o $(BUILD)/$(OUTPUT)

