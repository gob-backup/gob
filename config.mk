VERSION = \"0.4\"

PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin
MANDIR := $(DESTDIR)$(PREFIX)/share/man/man1

CC ?= gcc

CFLAGS   := -Wall -Wextra -pedantic -std=c90 -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L ${CFLAGS}
CPPFLAGS := -DGOB_VERSION=${VERSION} $(shell pkg-config --cflags libsodium) ${CPPFLAGS}
LDLIBS   := $(shell pkg-config --libs libsodium) ${LDLIBS}
