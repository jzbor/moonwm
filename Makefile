# moonwm - my own outstandingly named window manager
# See LICENSE file for copyright and license details.

include config.mk

VPATH = src
SRC = drw.c moonwm.c util.c
OBJ = ${SRC:.c=.o}

all: options moonwm

options:
	@echo moonwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

moonwm: ${OBJ}
	${CC} -g -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f moonwm ${OBJ} moonwm-${VERSION}.tar.gz

dist: clean
	mkdir -p moonwm-${VERSION}
	cp -R LICENSE Makefile README.md config.def.h config.mk\
		moonwm.1 drw.h util.h ${SRC} moonwm.png transient.c moonwm-${VERSION}
	tar -cf moonwm-${VERSION}.tar moonwm-${VERSION}
	gzip moonwm-${VERSION}.tar
	rm -rf moonwm-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f moonwm moonie moonwm-layoutmenu ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/moonwm
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < moonwm.1 > ${DESTDIR}${MANPREFIX}/man1/moonwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/moonwm.1
	mkdir -p ${DESTDIR}/usr/share/xsessions/
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < moonwm.desktop > ${DESTDIR}/usr/share/xsessions/moonwm.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm \
		${DESTDIR}${MANPREFIX}/man1/moonwm.1 \
		${DESTDIR}${PREFIX}/bin/moonie \
		${DESTDIR}${PREFIX}/bin/moonwm-layoutmenu
	rm -f ${DESTDIR}/usr/share/xsessions/moonwm.desktop

install-scripts:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f scripts/moonwm-helper ${DESTDIR}${PREFIX}/bin
	cp -f scripts/moonwm-menu ${DESTDIR}${PREFIX}/bin
	cp -f scripts/moonwm-status ${DESTDIR}${PREFIX}/bin
	cp -f scripts/moonwm-util ${DESTDIR}${PREFIX}/bin
	cp -f scripts/moonwm-xdg-xmenu ${DESTDIR}${PREFIX}/bin

uninstall-scripts:
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-helper
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-menu
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-status
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-util
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-xdg-xmenu

.PHONY: all options clean dist install install-scripts uninstall uninstall-scripts
