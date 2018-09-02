VERSION = \"0.5\"

PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin
MANDIR := $(DESTDIR)$(PREFIX)/share/man/man1

CFLAGS   ?= -Wall -Wextra -pedantic -std=c90
CPPFLAGS := -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DGOB_VERSION=${VERSION} `pkg-config --cflags libsodium`
LDLIBS   := `pkg-config --libs libsodium`
