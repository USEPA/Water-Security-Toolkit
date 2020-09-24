TEVAMODULEDIR = $(HOME)/EclipseWS/TEVA/teva-sp/packages/tevalibs/TEVAModule

OBJECTS = AggregationServerJNI.o \
IntAnalysisServerJNI.o \
namedData.o

all: $(OBJECTS) library

CC = gcc

CFLAGS = -g -fpic
LFLAGS = -shared -L/home/teva/TEVAFramework/libs/linux
IFLAGS = -I $(TEVAMODULEDIR)/include -I . \
         -I $(TEVAMODULEDIR)/ANLUtils/include

COMPILE.cc = $(CC)  -v -c $(CFLAGS) $(IFLAGS)


.c.o:
	$(COMPILE.cc) $<

clean:
	rm -f *.o

library:
	$(CC) -shared -o libAnalysisModule.so $(OBJECTS) $(LFLAGS) -lANLUtils

install:
	cp libAnalysisModule.so ../../.
	cp ../ANLUtils/lib/libANLUtils.so ../../.
