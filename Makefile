LANG_NAME=firtree
STYX=styx
CTOH=ctoh
CFLAGS=-I/usr/include/styx --std=c99 -Wall
CXXFLAGS=-I/usr/include/styx -Wall `llvm-config --cppflags`
LDFLAGS=-L/usr/lib `llvm-config --ldflags`
LIBS=-ldstyx `llvm-config --libs core`
GENDIR=gen

GENERATED_FILES= $(GENDIR)/$(LANG_NAME)_int.c \
	$(GENDIR)/$(LANG_NAME)_pim.c \
	$(GENDIR)/$(LANG_NAME)_lim.c \
	$(GENDIR)/$(LANG_NAME)_int.h \
	$(GENDIR)/$(LANG_NAME)_pim.h \
	$(GENDIR)/$(LANG_NAME)_lim.h \
	$(LANG_NAME).abs ctoh.cth 

GENERATED_C_SOURCES=$(filter %.c, $(GENERATED_FILES))

KERNELPARSE_FILES=$(GENERATED_FILES)\
	kernelparse.c

KERNELPARSE_C_SOURCES=$(filter %.c, $(KERNELPARSE_FILES))
KERNELPARSE_CXX_SOURCES=$(filter %.cc, $(KERNELPARSE_FILES))
KERNELPARSE_OBJECTS=$(KERNELPARSE_C_SOURCES:.c=.o) \
	$(KERNELPARSE_CXX_SOURCES:.cc=.o)

KERNELCOMPILE_FILES=$(GENERATED_FILES)\
	kernelcompile.cc llvmout.h llvmout.cc

KERNELCOMPILE_C_SOURCES=$(filter %.c, $(KERNELCOMPILE_FILES))
KERNELCOMPILE_CXX_SOURCES=$(filter %.cc, $(KERNELCOMPILE_FILES))
KERNELCOMPILE_OBJECTS=$(KERNELCOMPILE_C_SOURCES:.c=.o) \
	$(KERNELCOMPILE_CXX_SOURCES:.cc=.o)

all: kernelparse kernelcompile

parsetest: kernelparse
	./kernelparse testkernel.knl

clean:
	rm -f $(GENERATED_FILES)
	rm -f $(KERNELCOMPILE_OBJECTS)
	rm -f $(KERNELPARSE_OBJECTS)
	rm -f kernelparse
	rm -f kernelcompile

kernelparse: $(KERNELPARSE_FILES) $(KERNELPARSE_OBJECTS)
	$(CC) -o kernelparse $(KERNELPARSE_OBJECTS) $(LDFLAGS) $(LIBS)

kernelcompile: $(KERNELCOMPILE_FILES) $(KERNELCOMPILE_OBJECTS)
	$(CXX) -o kernelcompile $(KERNELCOMPILE_OBJECTS) $(LDFLAGS) $(LIBS)

$(GENDIR)/%_int.c $(GENDIR)/%_pim.c $(GENDIR)/%_lim.c %.abs: %.sty 
	mkdir -p $(GENDIR)
	GENSTYX=$(GENDIR) $(STYX) -makeC $*

$(GENDIR)/%_int.h $(GENDIR)/%_pim.h $(GENDIR)/%_lim.h: $(GENDIR)/%_int.c $(GENDIR)/%_pim.c $(GENDIR)/%_lim.c
	ctoh -CPATH=$(GENDIR) -HPATH=$(GENDIR)

.PHONY: all clean test

