CC = gcc
CFLAGS = -Wall
PROGRAMS = machine

ALL: ${PROGRAMS}

machine: machine.c
	${CC} ${CFLAGS} -o machine machine.c -pthread

clean:
	rm -f ${PROGRAMS}
