VERSION=2014-8

RM=/bin/rm
PREFIX?=/usr/local
LIB_SUFFIX?=lib
# MANPREFIX?=$(PREFIX)/share/man
SANEWM_PATH?=${PREFIX}/bin/sanewm
X11_INCLUDE?=/usr/local/include

#CC=clang
DIST=sanewm-$(VERSION)
SRC=sanewm.c window.c list.c pointer.c events.c desktop.c ewmh.c keys.c helpers.c monitor.c hidden.c config.h
# DISTFILES=Makefile README.md TODO sanewm.man $(SRC)
DISTFILES=Makefile README.md TODO $(SRC)
CFLAGS+=-Os -s -I${X11_INCLUDE} \
		-DSANEWM_PATH=\"${SANEWM_PATH}\"

LDFLAGS+=-L${PREFIX}/${LIB_SUFFIX} -lxcb -lxcb-randr -lxcb-keysyms \
		 -lxcb-icccm -lxcb-ewmh -lxcb-xrm
TARGETS=sanewm hidden
OBJS=sanewm.o window.o list.o events.o desktop.o pointer.o ewmh.o keys.o helpers.o monitor.o

all: $(TARGETS)

sanewm: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

hidden: hidden.c
	$(CC) -o $@ $(CFLAGS) hidden.c $(LDFLAGS)

sanewm.o: sanewm.c window.c list.c events.c desktop.c pointer.c ewmh.c keys.c helpers.c monitor.c config.h Makefile

install: $(TARGETS)
	test -d $(DESTDIR)$(PREFIX)/bin || mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -pm 755 sanewm $(DESTDIR)$(PREFIX)/bin
	install -pm 755 hidden $(DESTDIR)$(PREFIX)/bin
	test -d $(DESTDIR)$(MANPREFIX)/man1 || mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	install -pm 644 sanewm.man $(DESTDIR)$(MANPREFIX)/man1/sanewm.1
	install -pm 644 hidden.man $(DESTDIR)$(MANPREFIX)/man1/hidden.1

uninstall: deinstall
deinstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/sanewm
	$(RM) $(DESTDIR)$(PREFIX)/bin/hidden
	$(RM) $(DESTDIR)$(MANPREFIX)/man1/sanewm.1
	$(RM) $(DESTDIR)$(MANPREFIX)/man1/hidden.1

$(DIST).tar.bz2:
	mkdir $(DIST)
	cp $(DISTFILES) $(DIST)/
	tar cf $(DIST).tar --exclude .git $(DIST)
	bzip2 -9 $(DIST).tar
	$(RM) -rf $(DIST)

dist: $(DIST).tar.bz2

clean:
	$(RM) -f $(TARGETS) *.o

distclean: clean
	$(RM) -f $(DIST).tar.bz2
