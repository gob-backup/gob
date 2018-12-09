include config.mk

PROGRAMS=gob-cat gob-chunk gob-fsck
MANPAGES=${PROGRAMS:=.1}

CAT_SOURCES=gob-cat.c common.c
CAT_OBJECTS=${CAT_SOURCES:.c=.o}

CHUNK_SOURCES=gob-chunk.c common.c
CHUNK_OBJECTS=${CHUNK_SOURCES:.c=.o}

FSCK_SOURCES=gob-fsck.c common.c
FSCK_OBJECTS=${FSCK_SOURCES:.c=.o}

all: ${PROGRAMS}

gob-chunk: ${CHUNK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ ${CHUNK_OBJECTS} ${LDLIBS}

gob-cat: ${CAT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ ${CAT_OBJECTS} ${LDLIBS}

gob-fsck: ${FSCK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ ${FSCK_OBJECTS} ${LDLIBS}

.SUFFIXES: .c .o
.c.o: common.h config.h config.mk Makefile
	@echo "CC $@"
	@${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

.PHONY: test
test: ${PROGRAMS}
	@./test.sh

.PHONY: clean
clean:
	rm -f gob-cat ${CAT_OBJECTS}
	rm -f gob-chunk ${CHUNK_OBJECTS}

.PHONY: install
install:
	install -d -m 755 ${BINDIR}
	install -m 755 ${PROGRAMS} ${BINDIR}/
	install -d -m 755 ${MANDIR}
	install -m 644 ${MANPAGES} ${MANDIR}/
