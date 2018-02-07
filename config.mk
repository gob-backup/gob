PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin

CC ?= gcc

CFLAGS   := $(shell pkg-config --cflags libsodium) -Wall -Wextra -pedantic -std=c90 -D_POSIX_C_SOURCE=200809L ${CFLAGS}
LDFLAGS  := $(shell pkg-config --libs libsodium) ${LDFLAGS}
