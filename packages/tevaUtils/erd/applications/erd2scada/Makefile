srcdir = ./src
VPATH = $(srcdir)
homeprefix = $(HOME)

CC = gcc
CFLAGS = -g -I ./src 


.PHONY: all
all:runERD2SCADA

runERD2SCADA: src/runERD2SCADA.c liberd2scada.so
	${CC} ${CFLAGS} -fPIC -o runERD2SCADA src/runERD2SCADA.c -lerd2scada -lerd -lmysqlclient -lxml2 -L . -L$(homeprefix)/lib -I$(homeprefix)/include -I/usr/include/mysql/

liberd2scada.so: erd2scada.o
	${CC} ${CFLAGS} -fPIC -o liberd2scada.so erd2scada.o -shared

erd2scada.o: src/erd2scada.c src/erd2scada.h
	${CC} ${CFLAGS} -fPIC -c src/erd2scada.c



.PHONY: clean
clean:
	-rm *.so *.o runERD2SCADA

.PHONY: install
install:
	cp *.so $(homeprefix)/lib
	cp runERD2SCADA $(homeprefix)/bin
