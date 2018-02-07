PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin

CC ?= gcc

CFLAGS   := -Wall -Wextra -pedantic -std=c90 -D_POSIX_C_SOURCE=200809L ${CFLAGS}
CPPFLAGS := $(shell pkg-config --cflags libsodium) ${CPPFLAGS}
LDLIBS   := $(shell pkg-config --libs libsodium) ${LDLIBS}
