
.PHONY : clean all

TARGET := LCDserver
CC := gcc
CFLAGS := -Wall -Wextra
LDFLAGS := -lwiringPi

all: ${TARGET}

${TARGET}: LCDserver.c debian.xpm
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

clean:
	rm -f ${TARGET} *.o

