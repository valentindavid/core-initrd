include /usr/share/dpkg/buildflags.mk

all: detect-ubuntu-disk

EXTRA_CFLAGS:=$(shell pkg-config --cflags blkid icu-uc)
EXTRA_LIBS:=$(shell pkg-config --libs blkid icu-uc)
OBJECTS=detect-ubuntu-disk.o

detect-ubuntu-disk: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(EXTRA_LIBS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

clean:
	rm -f detect-ubuntu-disk $(OBJECTS)

.SUFFIXES: .c .o
.PHONY: clean
