bin_PROGRAMS=appserver
noinst_LIBRARIES=libutils.a

appserver_SOURCES=appserver.cpp preforked.cpp
libutils_a_SOURCES = \
	cregex.cpp utils.cpp parser.cpp cassigner.cpp endstate.cpp httpstate.cpp \
	scriptstate.cpp filestate.cpp mailstate.cpp querystate.cpp shellstate.cpp \
	structstate.cpp matchstate.cpp regexstate.cpp smsstate.cpp templates.cpp \
	gotostate.cpp cpgdatabase.cpp cdbmanager.cpp http.cpp

BOOST_LDADDS = @BOOST_LDFLAGS@ -lboost_program_options -lboost_filesystem -lboost_system
PQ_LDADDS = @LIBPQXX_LIBS@ -lpq

AM_CPPFLAGS=-I../include @UUID_CFLAGS@ @BOOST_CPPFLAGS@ @MEMCACHE_CFLAGS@ @LIBPQXX_CFLAGS@

appserver_LDFLAGS = -L. -lutils @UUID_LIBS@ @MEMCACHE_LIBS@ $(PQ_LDADDS) @FCGI_LDFLAGS@ \
	@LIBCURL_LIBS@ $(BOOST_LDADDS) @BSD_LIB@ @STDCXX_LIB@

$(bin_PROGRAMS): $(noinst_LIBRARIES)

clean-local:
	rm -f *~ *.o *.core *.a *.o-*

distclean-local:
	rm -rf .deps .dirstamp Makefile.in Makefile templates.cpp






