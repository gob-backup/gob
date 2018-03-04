include config.mk

PROGRAMS=gob-cat gob-chunk gob-decrypt gob-encrypt gob-fsck gob-keygen
MANPAGES=$(patsubst %,%.1,${PROGRAMS})

CAT_SOURCES=gob-cat.c common.c
CAT_OBJECTS=${CAT_SOURCES:%.c=%.o}

CHUNK_SOURCES=gob-chunk.c common.c
CHUNK_OBJECTS=${CHUNK_SOURCES:%.c=%.o}

DECRYPT_SOURCES=gob-decrypt.c common.c
DECRYPT_OBJECTS=${DECRYPT_SOURCES:%.c=%.o}

ENCRYPT_SOURCES=gob-encrypt.c common.c
ENCRYPT_OBJECTS=${ENCRYPT_SOURCES:%.c=%.o}

FSCK_SOURCES=gob-fsck.c common.c
FSCK_OBJECTS=${FSCK_SOURCES:%.c=%.o}

KEYGEN_SOURCES=gob-keygen.c common.c
KEYGEN_OBJECTS=${KEYGEN_SOURCES:%.c=%.o}

all: ${PROGRAMS}

gob-chunk: ${CHUNK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

gob-cat: ${CAT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

gob-decrypt: ${DECRYPT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

gob-encrypt: ${ENCRYPT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

gob-fsck: ${FSCK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

gob-keygen: ${KEYGEN_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

%.o: %.c common.h config.h config.mk Makefile
	@echo "CC $@"
	@${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

.PHONY: test
test: ${PROGRAMS}
	@./test.sh

.PHONY: clean
clean:
	rm -f gob-cat ${CAT_OBJECTS}
	rm -f gob-chunk ${CHUNK_OBJECTS}
	rm -f gob-decrypt ${DECRYPT_OBJECTS}
	rm -f gob-encrypt ${ENCRYPT_OBJECTS}
	rm -f gob-keygen ${KEYGEN_OBJECTS}

.PHONY: install
install:
	install -d -m 755 ${BINDIR}
	install -m 755 ${PROGRAMS} ${BINDIR}/
	install -d -m 755 ${MANDIR}
	install -m 644 ${MANPAGES} ${MANDIR}/
