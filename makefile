#!/usr/bin/make

VERSION = 1.3
DATE = $(shell date +%Y-%m-%d)

CC ?= gcc
AR ?= ar
CFLAGS += -fPIC

ifdef WITHOUT_LIBC
CFLAGS += -DZ3LIB_WITHOUT_LIBC='$(WITHOUT_LIBC)'
THREE = b d
else
THREE = b d f
endif

PREFIX ?= /usr
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include
MAN3DIR = $(PREFIX)/share/man/man3

Z3THREE = $(addprefix z3,$(THREE))
SRCSTEM = $(addsuffix lib,$(Z3THREE))
TRGSTEM = $(addprefix lib,$(Z3THREE))
SHAREDS = $(addsuffix .so,$(TRGSTEM))
STATICS = $(addsuffix .a,$(TRGSTEM))
TARGETS = $(SHAREDS) $(STATICS)
OBJECTS = $(addsuffix .o,$(SRCSTEM))
HEADERS = $(addsuffix .h,$(SRCSTEM)) z3lib.h
SOURCES = $(addsuffix .c,$(SRCSTEM))
TESTSD = $(addsuffix dtest,$(Z3THREE))
TESTSE = $(addsuffix etest,$(Z3THREE)) hashtest
TESTS = $(TESTSD) $(TESTSE)
ifdef ENCODE_ONLY
CFLAGS += -DZ3LIB_ENCODE_ONLY='$(ENCODE_ONLY)'
TESTS = $(TESTSE)
endif
ifdef DECODE_ONLY
CFLAGS += -DZ3LIB_DECODE_ONLY='$(DECODE_ONLY)'
TESTS = $(TESTSD)
endif
MAN3 = z3lib.3 z3dlib.3 z3flib.3
MAN = $(MAN3)
GENERATED = $(TARGETS) $(OBJECTS) $(TESTS) z3lib-$(VERSION).tar.gz
TESTADD = makefile z3crc32.h z3liblib.h
LICENCE = COPYING BSD-LICENCE
ALLSRC = $(LICENCE) $(TESTADD) $(SOURCES) $(HEADERS) $(addsuffix .c,$(TESTS))

.PHONY:	all shared static tests man install uninstall clean \
	install_shared install_static install_header targz \
	install_man

all:	shared static man

shared:	$(SHAREDS)

static:	$(STATICS)

tests:	$(TESTS)

man:	$(MAN)

$(SHAREDS):	lib%.so:	%lib.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -shared -o $@ $<

$(STATICS):	lib%.a:	%lib.o
	$(CROSS_COMPILE)$(AR) -cr $@ $<

$(OBJECTS):	%.o:	%.c $(HEADERS) $(TESTADD)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -O2 -c $< -o $@

$(TESTS):	%:	%.c $(SOURCES) $(HEADERS) $(TESTADD)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -O2 $< -o $@

install:	install_shared install_static install_header install_man

install_shared:	$(SHAREDS)
	install -d $(LIBDIR)
	install -c -m 755 $(SHAREDS) $(LIBDIR)

install_static:	$(STATICS)
	install -d $(LIBDIR)
	install -c -m 644 $(STATICS) $(LIBDIR)

install_header:	$(HEADERS)
	install -d $(INCDIR)
	install -c -m 644 $(HEADERS) $(INCDIR)

install_man:	$(MAN)
	install -d $(MAN3DIR)
	install -c -m 644 $(MAN3) $(MAN3DIR)

uninstall:
	cd $(LIBDIR) ; rm -vf $(TARGETS)
	cd $(INCDIR) ; rm -vf $(HEADERS)
	cd $(MAN3DIR) ; rm -vf $(MAN3)

targz:	$(ALLSRC) $(MAN)
	mkdir -p z3lib-$(VERSION)
	cp $(ALLSRC) z3lib-$(VERSION)/.
	for i in $(MAN) ; do \
	  sed -e 's/^\(.TH.*\)"DATE" "VERSION"/\1"$(DATE)" "$(VERSION)"/' \
	    <$$i >z3lib-$(VERSION)/$$i ; done
	tar -czf z3lib-$(VERSION).tar.gz z3lib-$(VERSION)
	rm -r z3lib-$(VERSION)

clean:
	rm -f $(GENERATED)
