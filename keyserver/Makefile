CC = clang

CFLAGS=-L/usr/lib/x86_64-linux-gnu/ -lX11 -lstdc++ -Wall -std=c++20 -Ofast -flto -fomit-frame-pointer -Iincludes

SOURCES := keyserver.cpp

all: keyserver

keyserver: $(addprefix source/,$(SOURCES))
	$(CC) $(CFLAGS) -o $@ $(addprefix source/,$(SOURCES))

clean:
	rm -f keyserver *.o

.PHONY: all clean
