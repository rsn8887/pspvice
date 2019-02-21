CC=psp-gcc
AR=psp-ar
RANLIB=psp-ranlib
RM=rm -rf
PSPSDK=$(shell psp-config --pspsdk-path)
CFLAGS=-G0 -O2 -Wall -DPSP -I$(PSPSDK)/include

all: libminizip.a

libminizip.a: ioapi.o unzip.o
	$(AR) cru $@ $?
	$(RANLIB) $@

clean:
	$(RM) *.o *.a
