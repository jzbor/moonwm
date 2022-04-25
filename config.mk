# moonwm version
VERSION = 8.1.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man
DOCPREFIX = ${PREFIX}/share/doc

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" -DXINERAMA
CFLAGS   = -std=c11 -pedantic -Wall -Wno-deprecated-declarations -O3 ${CPPFLAGS}
LDFLAGS  =

# libraries
MOONWM_DEPS	= fontconfig freetype2 freetype2 x11 x11-xcb xcb-res xft xinerama
MOONWM_LIBS = `pkg-config --libs $(MOONWM_DEPS)`
CFLAGS 	   += `pkg-config --cflags $(MOONWM_DEPS)`

MOONCTL_DEPS = x11
MOONCTL_LIBS = `pkg-config --libs $(MOONCTL_DEPS)`
CFLAGS 	   	+= `pkg-config --cflags $(MOONCTL_DEPS)`

# compiler, linker and md2man converter
CC 		= cc
MD2MAN	= go-md2man
