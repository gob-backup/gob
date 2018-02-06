PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin

CC ?= gcc

CFLAGS   := $(shell pkg-config --cflags libsodium) -Wall -Wextra -pedantic -std=c90 ${CFLAGS}
LDFLAGS  := $(shell pkg-config --libs libsodium) ${LDFLAGS}
