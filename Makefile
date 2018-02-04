include config.mk

SOURCES=sb-chunk.c
OBJECTS=${SOURCES:%.c=%.o}

sb-chunk: ${OBJECTS}
	@echo "LD $@"
	@${CC} ${LDFLAGS} -o $@ $<

%.o: %.c
	@echo "CC $@"
	@${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ $<

.PHONY: clean
clean:
	rm sb-chunk ${OBJECTS}
