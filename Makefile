include config.mk

PROGRAMS=sb-cat sb-chunk

CAT_SOURCES=sb-cat.c
CAT_OBJECTS=${CAT_SOURCES:%.c=%.o}

CHUNK_SOURCES=sb-chunk.c
CHUNK_OBJECTS=${CHUNK_SOURCES:%.c=%.o}

all: ${PROGRAMS}

sb-chunk: ${CHUNK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^

sb-cat: ${CAT_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $^

%.o: %.c config.h
	@echo "CC $@"
	@${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

.PHONY: clean
clean:
	rm -f sb-cat ${CAT_OBJECTS}
	rm -f sb-chunk ${CHUNK_OBJECTS}
