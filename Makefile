
# compiler configuration.
CC=gcc
CFLAGS=-g -O2 -fPIC -I./src -Wall -Wformat -Wno-strict-aliasing
LDLIBS=-lm
LDFLAGS=

# installer configuration.
INSTALL=install
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1

# julia configuration.
JL_CMD='println(joinpath(Sys.BINDIR,Base.DATAROOTDIR,"julia"))'
JL_SHARE=$(shell julia -e $(JL_CMD))
CFLAGS+= $(shell $(JL_SHARE)/julia-config.jl --cflags)
LDLIBS+= $(shell $(JL_SHARE)/julia-config.jl --ldlibs)
LDFLAGS+= $(shell $(JL_SHARE)/julia-config.jl --ldflags)

# binaries and objects to compile and link.
BIN=bin/gaputil bin/rejutil bin/jitutil
MAN=man/gaputil.1 man/rejutil.1 man/jitutil.1
OBJ=tup bst seq rej jit eval qrng
OBJS=$(addsuffix .o,$(addprefix src/,$(OBJ)))
BINOBJS=$(addsuffix .o,$(BIN))

# suffixes used in the build.
.SUFFIXES: .c .o

# all: default target.
all: check-julia $(BIN)

# gaputil: first executable linkage target.
bin/gaputil: $(OBJS) bin/gaputil.o
	@echo " LD $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# rejutil: second executable linkage target.
bin/rejutil: $(OBJS) bin/rejutil.o
	@echo " LD $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# jitutil: third executable linkage target.
bin/jitutil: $(OBJS) bin/jitutil.o
	@echo " LD $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# .c.o: compilation target.
.c.o:
	@echo " CC $^"
	@$(CC) $(CFLAGS) -c $^ -o $@

# install: installation target.
install: all
	@echo " INSTALL $(BIN)"
	@$(INSTALL) -d $(BINDIR)
	@$(INSTALL) -d $(MANDIR)
	@$(INSTALL) $(BIN) $(BINDIR)
	@$(INSTALL) $(MAN) $(MANDIR)

# clean: built file removal target.
clean:
	@echo " CLEAN"
	@rm -f $(BIN) $(OBJS) $(BINOBJS)

# again: repeat/rebuild compilation target.
again: clean all

# check-julia: target to check that a julia distribution exists.
check-julia:
	@echo " CHECK julia"
	@julia -v >/dev/null

# fixme: no-op target to check for fixme statements.
fixme:
	@echo " FIXME"
	@grep -RHni --color fixme src/*.[ch] bin/*.[ch] || echo " None found"

# lines: no-op target to perform a source code line count.
lines:
	@echo " WC"
	@wc -l src/*.[ch] bin/*.[ch] man/*.1

