CC = gcc
CFLAGS = -Wall
PROGRAMS = transport_protocol

ALL: ${PROGRAMS}

transport_protocol: main.o transport.o
	${CC} ${CFLAGS} -o transport_protocol main.o transport.o -pthread -lm -D_POSIX_C_SOURCE=1 -D_GNU_SOURCE

main.o: main.c transport.h
	${CC} ${CFLAGS} -c main.c transport.h -pthread -lm -D_POSIX_C_SOURCE=1 -D_GNU_SOURCE

transport.o: transport.c transport.h
	${CC} ${CFLAGS} -c transport.c transport.h -pthread -lm -D_POSIX_C_SOURCE=1 -D_GNU_SOURCE

clean:
	rm -f ${PROGRAMS}
