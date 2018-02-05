CC ?= gcc

CFLAGS   += -Wall -Wextra -pedantic -std=c90
CPPFLAGS += $(shell pkg-config --cflags libsodium)
LDFLAGS  += $(shell pkg-config --libs libsodium)
