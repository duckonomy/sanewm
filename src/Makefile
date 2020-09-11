VERSION=2018-7

RM=/bin/rm
PREFIX?=/usr/local
LIB_SUFFIX?=lib
SANEWM_PATH?=${PREFIX}/bin/sanewm
X11_INCLUDE?=/usr/local/include

#CC=clang
DIST=sanewm-$(VERSION)
SRC=sanewm.c window.c list.c pointer.c events.c desktop.c ewmh.c keys.c helpers.c monitor.c config.h
DISTFILES=Makefile README.md $(SRC)
CFLAGS+=-Os -s -I${X11_INCLUDE} \
		-DSANEWM_PATH=\"${SANEWM_PATH}\"

LDFLAGS+=-L${PREFIX}/${LIB_SUFFIX} -lxcb -lxcb-randr -lxcb-keysyms \
		 -lxcb-icccm -lxcb-ewmh
TARGETS=sanewm
OBJS=sanewm.o window.o list.o events.o desktop.o pointer.o ewmh.o keys.o helpers.o monitor.o

all: $(TARGETS)

sanewm: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

sanewm.o: sanewm.c window.c list.c events.c desktop.c pointer.c ewmh.c keys.c helpers.c monitor.c config.h Makefile

install: $(TARGETS)
	test -d $(DESTDIR)$(PREFIX)/bin || mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -pm 755 sanewm $(DESTDIR)$(PREFIX)/bin

uninstall: deinstall
deinstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/sanewm

clean:
	$(RM) -f $(TARGETS) *.o

distclean: clean
	$(RM) -f $(DIST).tar.bz2
