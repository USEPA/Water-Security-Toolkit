#
# Wrapper Makefile for Boost libraries
#

TOPBIN=${top_srcdir}/bin

# Make sure this build of boost uses the headers included with it
AM_CPPFLAGS=-I${top_srcdir}
# This was defined in the cmake, but we didn't carry it
#AM_CPPFLAGS+=-DBOOST_SIGNALS_NO_LIB=1

lib_LTLIBRARIES=libboost_signals.la

libboost_signals_la_SOURCES= \
 signals/src/trackable.cpp \
 signals/src/connection.cpp  \
 signals/src/named_slot_map.cpp  \
 signals/src/signal_base.cpp  \
 signals/src/slot.cpp

MAINTAINERCLEANFILES = Makefile.in

# Build the Boost linkable libraries, and place them in a location that
# mimics the one that the native Boost build system uses (so users can
# use their own Boost snapshot with --with-boost=<boost_root>
all-local: $(lib_LTLIBRARIES)
	@mkdir -p ../stage/lib
	@cp .libs/*.a ../stage/lib

clean-local:
	rm -f *.gcno *.gcda

copy-headers:

clean-headers:

