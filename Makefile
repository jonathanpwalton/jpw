program = jpw
version = 20250423

objects = jpw.o print.o pull.o curl.o

CC = gcc
CPPFLAGS = -DNDEBUG
CFLAGS = -Os -s -Wall -Wextra -pedantic -Wno-unused-value -Wno-unused-parameter -Wno-unused-variable
LD = $(CC)
LDFLAGS = $(CFLAGS)
LIBS = -lcurl

.PHONY: clean

$(program): $(objects)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: src/%.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(program) *.o
