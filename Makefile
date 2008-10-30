LANG_NAME=firtree
STYX=styx
CTOH=ctoh
LLVM_MAJOR=`llvm-config --version | cut -d . -f 1`
LLVM_MINOR=`llvm-config --version | cut -d . -f 2`
COMMON_FLAGS=-DLLVM_MAJOR_VER=$(LLVM_MAJOR) -DLLVM_MINOR_VER=$(LLVM_MINOR) -I/usr/include/styx -Wall -g
CFLAGS=--std=c99 $(COMMON_FLAGS)
CXXFLAGS=$(COMMON_FLAGS) `llvm-config --cppflags` `pkg-config firtree --cflags`
LDFLAGS=-L/usr/lib `llvm-config --ldflags`
LIBS=`llvm-config --libs core bitwriter` -ldstyx `pkg-config firtree --libs`
GENDIR=gen

GENERATED_FILES= $(GENDIR)/$(LANG_NAME)_int.c \
	$(GENDIR)/$(LANG_NAME)_pim.c \
	$(GENDIR)/$(LANG_NAME)_lim.c \
	$(GENDIR)/$(LANG_NAME)_int.h \
	$(GENDIR)/$(LANG_NAME)_pim.h \
	$(GENDIR)/$(LANG_NAME)_lim.h \
	$(GENDIR)/$(LANG_NAME).abs ctoh.cth 

GENERATED_C_SOURCES=$(filter %.c, $(GENERATED_FILES))

KERNELPARSE_FILES=$(GENERATED_FILES)\
	kernelparse.c

KERNELPARSE_C_SOURCES=$(filter %.c, $(KERNELPARSE_FILES))
KERNELPARSE_CXX_SOURCES=$(filter %.cc, $(KERNELPARSE_FILES))
KERNELPARSE_OBJECTS=$(KERNELPARSE_C_SOURCES:.c=.o) \
	$(KERNELPARSE_CXX_SOURCES:.cc=.o)

KERNELCOMPILE_FILES=$(GENERATED_FILES)\
	kernelcompile.cc \
	llvm_backend/llvm_backend.h \
	llvm_backend/llvm_private.h \
	llvm_backend/llvm_backend.cc \
	llvm_backend/llvm_emit_decl.h llvm_backend/llvm_emit_decl.cc \
	llvm_backend/llvm_expression.h \
	llvm_backend/llvm_expression.cc \
	llvm_backend/llvm_type_cast.h llvm_backend/llvm_type_cast.cc \
	llvm_backend/llvm_emit_return.cc \
	llvm_backend/llvm_emit_expr_list.cc \
	llvm_backend/llvm_emit_constant.h llvm_backend/llvm_emit_constant.cc \
	llvm_backend/llvm_emit_function_call.cc \
	llvm_backend/llvm_emit_variable_declaration.cc \
	llvm_backend/llvm_emit_variable.cc \
	llvm_backend/llvm_emit_binop_arith.cc \
	llvm_backend/llvm_emit_binop_logic.cc \
	llvm_backend/llvm_emit_binop_cmp.cc \
	llvm_backend/llvm_emit_binop_assign.cc \
	llvm_backend/symbol_table.cc
#	llvmout.h llvmout.cc \
#	llvmutil.h llvmutil.cc \
#	llvmout_priv.h llvmexpout.cc \

KERNELCOMPILE_C_SOURCES=$(filter %.c, $(KERNELCOMPILE_FILES))
KERNELCOMPILE_CXX_SOURCES=$(filter %.cc, $(KERNELCOMPILE_FILES))
KERNELCOMPILE_OBJECTS=$(KERNELCOMPILE_C_SOURCES:.c=.o) \
	$(KERNELCOMPILE_CXX_SOURCES:.cc=.o)

all: kernelparse kernelcompile

beautify:
	for file in $(filter %.cc, $(KERNELCOMPILE_FILES)); do \
		astyle -s4 -t4 -l -C -S -w -U -D "$$file"; \
		rm -f "$${file}.orig"; \
	done
	for file in $(filter %.h, $(KERNELCOMPILE_FILES)); do \
		astyle -s4 -t4 -l -C -S -w -U -D "$$file"; \
		rm -f "$${file}.orig"; \
	done

compiletest: kernelcompile
	./kernelcompile testkernel.knl >testkernel.llo
	@echo "=========> GENERATED CODE <========="
	llvm-dis -o - testkernel.llo
	@echo "=========> OPTIMISED CODE <========="
	opt -std-compile-opts testkernel.llo | llvm-dis -o -

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

$(GENDIR)/%_int.c $(GENDIR)/%_pim.c $(GENDIR)/%_lim.c $(GENDIR)/%.abs: %.sty 
	mkdir -p $(GENDIR)
	GENSTYX=$(GENDIR) $(STYX) -makeC $*

$(GENDIR)/%_int.h $(GENDIR)/%_pim.h $(GENDIR)/%_lim.h: $(GENDIR)/%_int.c $(GENDIR)/%_pim.c $(GENDIR)/%_lim.c
	ctoh -CPATH=$(GENDIR) -HPATH=$(GENDIR)

.PHONY: all beautify clean parsetest compiletest

