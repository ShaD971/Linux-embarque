CC ?= cc
CFLAGS ?= -O2

.PHONY: all clean

all: helloworld

helloworld: helloworld.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f helloworld
