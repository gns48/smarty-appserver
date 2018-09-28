SUBDIRS = include src conf test doc script
automake_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

package:
if BUILDING_RPM
	@echo "Creating RPM $(PACKAGE)-$(VERSION).rpm"
endif
if BUILDING_DEB
	@echo "Creating Debian package $(PACKAGE)-$(VERSION).deb"
endif
if BUILDING_FBSD 
	@echo "Creating FreeBSD package $(PACKAGE)-$(VERSION).pkg"
else
	@echo "You are trying to build FreeBSD package on the other OS!"
	@exit 1
endif

clean-local:
	@rm -f *~  *.bak

distclean-local:
	@rm -rf configure *.log Makefile.in aclocal.m4 config.h.in config.h \
		ar-lib compile install-sh missing depcomp ylwrap autom4te.cache \
		config.sub config.guess ltmain.sh \
		m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 m4/lt~obsolete.m4












