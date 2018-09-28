dnl
dnl Check for bison 
dnl AC_PROG_BISON([MIN_VERSION=2.0])
dnl
AC_DEFUN([AC_PROG_BISON], [
if test "x$1" = "x" ; then
	bison_required_version="2.0"
else 
  	bison_required_version="$1"
fi
AC_CHECK_PROG(have_prog_bison, [bison], [yes],[no])
if test "$have_prog_bison" = "yes" ; then
	AC_MSG_CHECKING([for bison version = $bison_required_version])
	bison_version=`bison --version | head -n 1 | cut '-d ' -f 4`
	if test "$bison_version" = "$bison_required_version" ; then
		AC_MSG_RESULT([yes])
		BISON=bison
                AC_SUBST(BISON)
	else
		BISON=:
		AC_MSG_RESULT([no])
	fi
else
	BISON=:
	AC_MSG_RESULT([NO])
fi
AC_SUBST(BISON)
])
