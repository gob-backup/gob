VERSION = \"0.5\"

PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin
MANDIR := $(DESTDIR)$(PREFIX)/share/man/man1

CC ?= gcc

CFLAGS   := -Wall -Wextra -pedantic -std=c90 ${CFLAGS}
CPPFLAGS := -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DGOB_VERSION=${VERSION} $(shell pkg-config --cflags libsodium) ${CPPFLAGS}
LDLIBS   := $(shell pkg-config --libs libsodium) ${LDLIBS}
