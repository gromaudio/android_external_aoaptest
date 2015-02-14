# standalone Makefile for adb
SRCS+= aoap_test.c

CPPFLAGS+= -I.
CFLAGS+= -O2 -g -Wall -Wno-unused-parameter
LIBS= -lusb-1.0
TOOLCHAIN=
CC= $(TOOLCHAIN)gcc
LD= $(TOOLCHAIN)gcc

OBJS= $(SRCS:.c=.o)

all: aoap_test

aoap_test: $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBB) $(LIBS)

clean:
	rm -rf $(OBJS)
