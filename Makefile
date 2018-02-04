include config.mk

PROGRAMS=sb-chunk

CHUNK_SOURCES=sb-chunk.c
CHUNK_OBJECTS=${CHUNK_SOURCES:%.c=%.o}

all: ${PROGRAMS}

sb-chunk: ${CHUNK_OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $<

%.o: %.c config.h
	@echo "CC $@"
	@${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

.PHONY: clean
clean:
	rm -f sb-chunk ${CHUNK_OBJECTS}
