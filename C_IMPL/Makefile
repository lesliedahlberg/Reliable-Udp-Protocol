CC = gcc
CFLAGS = -Wall -w
PROGRAMS = machine

ALL: ${PROGRAMS}

machine: machine.c
	${CC} ${CFLAGS} -o machine machine.c -pthread

clean:
	rm -f ${PROGRAMS}
