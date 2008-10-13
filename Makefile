LANG_NAME=firtree
STYX=styx
CTOH=ctoh
CFLAGS=-I/usr/include/styx --std=c99 -Wall
CXXFLAGS=$(CFLAGS)
LDFLAGS=-L/usr/lib

GENERATED_FILES= $(LANG_NAME)_int.c \
	$(LANG_NAME)_pim.c \
	$(LANG_NAME)_lim.c \
	$(LANG_NAME)_int.h \
	$(LANG_NAME)_pim.h \
	$(LANG_NAME)_lim.h \
	$(LANG_NAME).abs ctoh.cth 

GENERATED_C_SOURCES=$(filter %.c, $(GENERATED_FILES))

KERNELPARSE_FILES=$(GENERATED_FILES)\
	kernelparse.c

KERNELPARSE_C_SOURCES=$(filter %.c, $(KERNELPARSE_FILES))
KERNELPARSE_CXX_SOURCES=$(filter %.cc, $(KERNELPARSE_FILES))
KERNELPARSE_OBJECTS=$(KERNELPARSE_C_SOURCES:.c=.o) \
	$(KERNELPARSE_CXX_SOURCES:.cc=.o)

KERNELCOMPILE_FILES=$(GENERATED_FILES)\
	kernelcompile.cc

KERNELCOMPILE_C_SOURCES=$(filter %.c, $(KERNELCOMPILE_FILES))
KERNELCOMPILE_CXX_SOURCES=$(filter %.cc, $(KERNELCOMPILE_FILES))
KERNELCOMPILE_OBJECTS=$(KERNELCOMPILE_C_SOURCES:.c=.o) \
	$(KERNELCOMPILE_CXX_SOURCES:.cc=.o)

all: kernelparse kernelcompile

parsetest: kernelparse
	./kernelparse testkernel.knl

clean:
	rm -f $(GENERATED_FILES)
	rm -f kernelparse.o

kernelparse: $(KERNELPARSE_FILES) $(KERNELPARSE_OBJECTS)
	$(CC) -o kernelparse $(KERNELPARSE_OBJECTS) $(LDFLAGS) -ldstyx

kernelcompile: $(KERNELCOMPILE_FILES) $(KERNELCOMPILE_OBJECTS)
	$(CXX) -o kernelcompile $(KERNELCOMPILE_OBJECTS) $(LDFLAGS) -ldstyx

%_int.c %_pim.c %_lim.c %.abs: %.sty
	$(STYX) -makeC $*

%_int.h %_pim.h %_lim.h: %_int.c %_pim.c %_lim.c
	ctoh -api=$*

.PHONY: all clean test

