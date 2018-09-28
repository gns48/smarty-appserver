noinst_PROGRAMS=cregextest writepid assigntest fcgitest jsontest
cregextest_SOURCES=cregextest.cpp 
writepid_SOURCES=writepid.cpp
assigntest_SOURCES=assigntest.cpp
fcgitest_SOURCES=fcgi_test.cpp
jsontest_SOURCES=json_test.cpp

AM_CPPFLAGS=-I../include @BOOST_CPPFLAGS@  @FCGI_CXXFLAGS@

$(noinst_PROGRAMS): ../src/libutils.a

EXTRA_LIBS = -L../src -lutils @UUID_LIBS@ @MEMCACHE_LIBS@ @LIBPQXX_LIBS@ -lpq @FCGI_LDFLAGS@ @LIBCURL_LIBS@
BOOST_LDADDS = @BOOST_LDFLAGS@ -lboost_program_options -lboost_filesystem -lboost_system

cregextest_LDFLAGS = $(EXTRA_LIBS) $(BOOST_LDADDS) @STDCXX_LIB@
writepid_LDFLAGS = $(EXTRA_LIBS) @STDCXX_LIB@
assigntest_LDFLAGS = $(EXTRA_LIBS) $(BOOST_LDADDS) @STDCXX_LIB@
fcgitest_LDFLAGS= @FCGI_LDFLAGS@  @STDCXX_LIB@
jsontest_LDFLAGS = @BOOST_LDFLAGS@ @STDCXX_LIB@

test: test-re test-pid 

test-re:
	echo "=== running $@ ==="
	./cregextest ../conf/regexlib.dat ./data/regextest.dat 

test-pid:
	echo "=== running $@ ==="
	echo "#!`which bash`" > testpid.sh
	echo './writepid pid-test.dat &' >> testpid.sh
	echo 'echo $$! > pid-shell.dat' >> testpid.sh
	echo "sleep 1" >> testpid.sh 
	echo "diff pid-test.dat pid-shell.dat" >> testpid.sh 
	chmod 755 testpid.sh
	./testpid.sh

test-cgi:
	echo "=== running $@ ==="
	echo "Pleasae configure Your web server to enable fast cgi redirect to port 9191"
	./fcgitest :9191

clean-local:
	rm -f *~ *.dat *.core testpid.sh *.out

distclean-local:
	rm -rf .deps Makefile Makefile.in *.out










