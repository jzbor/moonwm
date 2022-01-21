# moonwm - my own outstandingly named window manager
# See LICENSE file for copyright and license details.

include config.mk

VPATH = src
SRC = drw.c moonwm.c util.c xwrappers.c
OBJ = ${SRC:.c=.o}

all: options moonwm moonctl

options:
	@echo moonwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk common.h

config.h:
	cp config.def.h $@

moonwm: ${OBJ}
	${CC} -g -o $@ ${OBJ} ${LDFLAGS}

moonctl: moonctl.c
	${CC} -g -o $@ src/moonctl.c ${LDFLAGS}

clean:
	rm -f moonctl moonwm ${OBJ} moonctl.o moonwm-${VERSION}.tar.gz

dist: clean
	mkdir -p moonwm-${VERSION}
	cp -R LICENSE Makefile CHANGELOG.md README.md config.def.h config.mk\
		moonwm.1 drw.h util.h ${SRC} moonwm.png transient.c moonwm-${VERSION}
	tar -cf moonwm-${VERSION}.tar moonwm-${VERSION}
	gzip moonwm-${VERSION}.tar
	rm -rf moonwm-${VERSION}

install: all
	install -Dm755 moonwm moonctl -t ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < moonwm.1 > ${DESTDIR}${MANPREFIX}/man1/moonwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/moonwm.1
	install -Dm644 CHANGELOG.md README.md -t ${DESTDIR}${DOCPREFIX}/moonwm/
	mkdir -p ${DESTDIR}${PREFIX}/share/xsessions/
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < moonwm.desktop > ${DESTDIR}${PREFIX}/share/xsessions/moonwm.desktop
	install -Dm644 moonwm-wallpaper.desktop -t ${DESTDIR}${PREFIX}/share/applications/

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm \
		${DESTDIR}${MANPREFIX}/man1/moonwm.1 \
		${DESTDIR}${PREFIX}/bin/moonctl
	rm -f ${DESTDIR}${PREFIX}/share/xsessions/moonwm.desktop \
		${DESTDIR}${PREFIX}/share/applications/moonwm-wallpaper.desktop

install-scripts:
	install -Dm755 scripts/moonwm-helper scripts/moonwm-menu scripts/moonwm-status scripts/wmc-utils scripts/xdg-xmenu -t ${DESTDIR}${PREFIX}/bin
	ln -sf ${DESTDIR}${PREFIX}/bin/wmc-utils ${DESTDIR}${PREFIX}/bin/moonwm-utils

uninstall-scripts:
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-helper
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-menu
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-status
	rm -f ${DESTDIR}${PREFIX}/bin/wmc-utils
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-utils
	rm -f ${DESTDIR}${PREFIX}/bin/xdg-xmenu

pull-xdg-xmenu:
	wget https://raw.githubusercontent.com/jzbor/mashup/master/utils/xdg-xmenu
	mv xdg-xmenu scripts/xdg-xmenu

.PHONY: all options clean dist install install-scripts uninstall uninstall-scripts pull-xdg-xmenu
