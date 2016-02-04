CC = gcc
CFLAGS = -Wall
PROGRAMS = machine

ALL: ${PROGRAMS}

machine: machine.c
	${CC} ${CFLAGS} -o machine machine.c -pthread -lm -D_POSIX_C_SOURCE=1 -D_GNU_SOURCE 

clean:
	rm -f ${PROGRAMS}
