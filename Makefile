LANG_NAME=firtree
STYX=styx
CTOH=ctoh
CFLAGS=-I/usr/include/styx
LDFLAGS=-L/usr/lib

GENERATED_SOURCES= $(LANG_NAME)_int.c \
	$(LANG_NAME)_pim.c \
	$(LANG_NAME)_lim.c 

GENERATED_FILES = $(GENERATED_SOURCES) \
	$(LANG_NAME)_int.h \
	$(LANG_NAME)_pim.h \
	$(LANG_NAME)_lim.h \
	$(LANG_NAME).abs ctoh.cth 

KERNELPARSE_FILES=$(GENERATED_SOURCES)\
	kernelparse.c

all: kernelparse

test: kernelparse
	./kernelparse testkernel.knl

clean:
	rm -f $(GENERATED_FILES)
	rm -f kernelparse.o

kernelparse.c: $(GENERATED_FILES)

kernelparse: $(KERNELPARSE_FILES:.c=.o)
	gcc -o kernelparse $^ $(LDFLAGS) -ldstyx

%_int.c %_pim.c %_lim.c %.abs: %.sty
	$(STYX) -makeC $*

%_int.h %_pim.h %_lim.h: %_int.c %_pim.c %_lim.c
	ctoh -api=$*

.PHONY: all clean test

