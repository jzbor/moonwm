# moonwm - my own outstandingly named window manager
# See LICENSE file for copyright and license details.

include config.mk

VPATH = src scripts
MOONWM_OBJECTS 	= drw.o moonwm.o util.o xwrappers.o
MOONCTL_OBJECTS = moonctl.o

all: options moonwm moonctl moonwm.1

options:
	@echo moonwm build options:
	@echo "CFLAGS   	= ${CFLAGS}"
	@echo "LDFLAGS  	= ${LDFLAGS}"
	@echo "CC       	= ${CC}"
	@echo "MOONWM_LIBS	= ${MOONWM_LIBS}"
	@echo "MOONCTL_LIBS	= ${MOONCTL_LIBS}"

%.o: %.c config.h rules.h config.mk
	${CC} -c ${CFLAGS} $<

config.h: config.def.h
	cp -a src/config.def.h src/config.h

rules.h:
	cp -a src/rules.def.h src/rules.h

moonwm: ${MOONWM_OBJECTS}
	${CC} -g -o $@ $^ ${MOONWM_LIBS} ${LDFLAGS}

moonctl: ${MOONCTL_OBJECTS}
	${CC} -g -o $@ $^ ${MOONCTL_LIBS} ${LDFLAGS}

moonwm.1: README.md
	go-md2man -in $< -out $@

clean:
	rm -f moonctl moonwm moonwm-${VERSION}.tar.gz
	rm -f ${MOONWM_OBJECTS} ${MOONCTL_OBJECTS}
	rm -f *.1
	rm -f src/config.h src/rules.h

dist: clean
	mkdir -p moonwm-${VERSION}
	cp -R LICENSE Makefile CHANGELOG.md README.md config.def.h config.mk\
		moonwm.1 drw.h util.h ${SRC} moonwm.png transient.c moonwm-${VERSION}
	tar -cf moonwm-${VERSION}.tar moonwm-${VERSION}
	gzip moonwm-${VERSION}.tar
	rm -rf moonwm-${VERSION}

install: moonwm moonctl
	install -Dm755 moonwm moonctl -t ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/share/xsessions/
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < moonwm.desktop > ${DESTDIR}${PREFIX}/share/xsessions/moonwm.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm \
		${DESTDIR}${PREFIX}/bin/moonctl
	rm -f ${DESTDIR}${PREFIX}/share/xsessions/moonwm.desktop

install-scripts: moonwm-helper moonwm-menu moonwm-status moonwm-utils
	install -Dm755 scripts/moonwm-helper scripts/moonwm-menu scripts/moonwm-status scripts/moonwm-utils -t ${DESTDIR}${PREFIX}/bin

uninstall-scripts:
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-helper
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-menu
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-status
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-utils
	rm -f ${DESTDIR}${PREFIX}/bin/moonwm-utils

install-docs: README.md CHANGELOG.md moonwm.1
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < moonwm.1 > ${DESTDIR}${MANPREFIX}/man1/moonwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/moonwm.1
	install -Dm644 CHANGELOG.md README.md -t ${DESTDIR}${DOCPREFIX}/moonwm/

uninstall-docs:
	rm -rf ${DESTDIR}${DOCPREFIX}/moonwm
	rm -f ${DESTDIR}${MANPREFIX}/man1/moonwm.1

install-all: install install-scripts install-docs

uninstall-all: uninstall uninstall-scripts uninstall-docs

.PHONY: all options clean dist install install-scripts uninstall uninstall-scripts install-all \
	uninstall-all install-docs uninstall-docs
.NOTPARALLEL: clean
