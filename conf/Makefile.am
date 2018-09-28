appserverdir = $(datadir)/appserver
appserver_DATA = regexlib.dat

appconfdir = $(sysconfdir)
appconf_DATA = appserver.conf

initddir = $(sysconfdir)/@INITD@
initd_SCRIPTS = appserver

$(initd_SCRIPTS):
	mv $@.`uname` $@

clean-local:
	rm -f *~ *.bak 

distclean-local:
	rm -f Makefile.in appserver.`uname` appserver


