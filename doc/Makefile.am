if HAVE_DOXYGEN
doc:
	doxygen > doxygen.log 2>&1 
else
doc:
	@echo "Doxygen is not installed!"
	@exit 1
endif

clean-local:
	rm -rf *~ *.bak doxygen.log html latex

distclean-local:
	rm -rf Makefile.in Doxyfile html latex








