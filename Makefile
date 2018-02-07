include config.mk

PROGRAMS=sb-cat sb-chunk sb-decrypt sb-encrypt sb-keygen

CAT_SOURCES=sb-cat.c common.c
CAT_OBJECTS=${CAT_SOURCES:%.c=%.o}

CHUNK_SOURCES=sb-chunk.c common.c
CHUNK_OBJECTS=${CHUNK_SOURCES:%.c=%.o}

DECRYPT_SOURCES=sb-decrypt.c common.c
DECRYPT_OBJECTS=${DECRYPT_SOURCES:%.c=%.o}

ENCRYPT_SOURCES=sb-encrypt.c common.c
ENCRYPT_OBJECTS=${ENCRYPT_SOURCES:%.c=%.o}

KEYGEN_SOURCES=sb-keygen.c common.c
KEYGEN_OBJECTS=${KEYGEN_SOURCES:%.c=%.o}

all: ${PROGRAMS}

sb-chunk: ${CHUNK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

sb-cat: ${CAT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

sb-decrypt: ${DECRYPT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

sb-encrypt: ${ENCRYPT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

sb-keygen: ${KEYGEN_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^ ${LDLIBS}

%.o: %.c config.h
	@echo "CC $@"
	@${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

.PHONY: test
test: ${PROGRAMS}
	@./test.sh

.PHONY: clean
clean:
	rm -f sb-cat ${CAT_OBJECTS}
	rm -f sb-chunk ${CHUNK_OBJECTS}
	rm -f sb-decrypt ${DECRYPT_OBJECTS}
	rm -f sb-encrypt ${ENCRYPT_OBJECTS}
	rm -f sb-keygen ${KEYGEN_OBJECTS}

.PHONY: install
install:
	install -d -m 755 ${BINDIR}
	install -m 755 ${PROGRAMS} ${BINDIR}/
