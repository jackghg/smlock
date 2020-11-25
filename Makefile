# smlock
include config.mk

SRC = smlock.c
OBJ = ${SRC:.c=.o}

all: options smlock

options:
	@echo smlock build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

smlock: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f smlock ${OBJ} smlock-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p smlock-${VERSION}
	@cp -R LICENSE Makefile README config.mk ${SRC} smlock-${VERSION}
	@tar -cf smlock-${VERSION}.tar smlock-${VERSION}
	@gzip smlock-${VERSION}.tar
	@rm -rf smlock-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f smlock ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/smlock
	@chmod u+s ${DESTDIR}${PREFIX}/bin/smlock

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/smlock

.PHONY: all options clean dist install uninstall
