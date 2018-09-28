LC_ALL=C

AC_PREREQ([2.69])
AC_INIT([appserver], [2.1], [Gleb N. Semenov <gleb.semenov@gmail.com>])
AC_LANG([C++])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/appserver.cpp])
AC_CONFIG_HEADER([config.h])

# default prefix for installs
AC_PREFIX_DEFAULT(/usr/local)

dnl Cheching for OS type
AC_CANONICAL_HOST

# Checks for programs.
AM_PROG_AR
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

AC_PATH_PROG([MKDIR], [mkdir], [no])
AC_PATH_PROG([PKG_CONFIG], [pkg-config], [no])
AC_PATH_PROG([TEST], [test], [no])
# Optional programs for packaging
AC_PATH_PROG([DOT], [dot], [no])
AC_PATH_PROG([RPMBUILD], [rpmbuild], [no])
AC_PATH_PROG([RPM], [rpm], [no])
AC_PATH_PROG([DEBUILD], [dpkg-buildpackage], [no])

AC_ISC_POSIX
AC_HEADER_STDC

dnl --- packaging staff ---
dnl --- RPM ---
# for use in building RPM packages. Requires rpm and rpmbuild.
AC_ARG_ENABLE([rpm], 
              [AS_HELP_STRING([--enable-rpm @=<:@yes/no@:>@],
                 [Configure for building RPM package @<:@default=no@:>@(requires rpm and rpmbuild)])],
              [enable_rpm=yes], [enable_rpm=no])
if test "x$enable_rpm" = xyes ; then
   if test "x$host_os" != xlinux-gnu ; then AC_MSG_ERROR([--enable-rpm: Run this on Linux!]) ; fi
   if test "x$RPMBUILD" = xno ; then AC_MSG_ERROR([rpmbuild is required for --enable-rpm]) ; fi
   if test "x$RPM" = xno ; then AC_MSG_ERROR([rpm is required for --enable-rpm]) ; fi
fi
AM_CONDITIONAL(BUILDING_RPM, test "x$enable_rpm" = xyes)

dnl --- Debian ---
dnl Requires dpkg-buildpackage
AC_ARG_ENABLE([deb], 
              [AS_HELP_STRING([--enable-deb @=<:@yes/no@:>@],
                  [Configure for building deb package @<:@default=no@:>@ (requires dpkg-buildpackage)])],
              [enable_deb=yes], [enable_deb=no])
if test "x$enable_deb" = xyes ; then
   if test "x$host_os" != xlinux-gnu ; then AC_MSG_ERROR([--enable-deb: Run this on Linux!]) ; fi
   if test "x$DEBUILD" = xno ; then AC_MSG_ERROR([dpkg-buildpackage is required for --enable-deb]) ; fi
fi
AM_CONDITIONAL([BUILDING_DEB], [$TEST "x$enable_deb" = xyes])

dnl --- FreeBSD ---
dnl Possible only in FreeBSD environment
AC_ARG_ENABLE([fbsd], 
              [AS_HELP_STRING([--enable-fbsd @=@<:@yes/no@:>@],
                  [Configure for building FreeBSD package @<:@default=no@:>@ (requires FreeBSD)])],
              [enable_fbsd=yes], [enable_fbsd=no])
if test "x$enable_fbsd" = xyes ; then
   if test "x$host_os" != xfreebsd ; then AC_MSG_ERROR([--enable-fbsd: Run this on FreeBSD!]) ; fi
fi
AM_CONDITIONAL([BUILDING_FBSD], [$TEST "x$enable_fbsd" = xyes])

dnl reset CXXFLAGS
test -z "$CXXFLAGS" && CXXFLAGS='-O2'
test -z "$CFLAGS" && CFLAGS='-O2'

dnl Cheching for OS type
AC_CANONICAL_HOST
case $host_os in
    linux*) INITD=init.d ;;
    *bsd*)  INITD=rc.d ;;
    *)      AC_MSG_ERROR([Your platform is not currently supported]) ;;
esac
AC_SUBST(INITD)

dnl Check for C++ 11 support
AX_CHECK_COMPILE_FLAG([-std=c++0x], [cxx11="-std=c++0x"])
AX_CHECK_COMPILE_FLAG([-std=c++11], [cxx11="-std=c++11"])
AM_CONDITIONAL([HAVE_CXX11], [test -n "$cxx11"])
AM_COND_IF([HAVE_CXX11], [CXXFLAGS="$CXXFLAGS $cxx11"],
                         [AC_MSG_ERROR([No C++11-aware compiler detected!])])
dnl set warning level
AC_ARG_WITH(warning,
            [  --with-warning=[compiler warning option]
                              [set compiler warning option]
                              [default -Wall]], 
            with_warning=${withval}, with_warning="all")
CXXFLAGS="$CXXFLAGS -W$with_warning"

dnl enable debug toggle
AC_ARG_ENABLE(debug,[  --enable-debug    Turn on debugging],
        [case "${enableval}" in
                yes) debug=true ;;
                no)  debug=false ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
         esac], 
         [debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])
AM_COND_IF([DEBUG],
           [CXXFLAGS="$CXXFLAGS -g"
            AC_DEFINE([DEBUG], [], [Ddebug enabled])], [])

AX_BOOST_BASE([1.52], ,AC_MSG_ERROR([No boost libraries found!]))

dnl check for memcached
PKG_CHECK_MODULES(MEMCACHE, libmemcached >= 1.0.1)
AC_SUBST(MEMCACHE_CFLAGS)
AC_SUBST(MEMCACHE_LIBS)
AC_DEFINE([HAVE_MEMCACHE], [], [libmemcached is found and operational])

dnl check for libcurl
PKG_CHECK_MODULES(LIBCURL, [libcurl >= 7.0])
AC_SUBST(LIBCURL_CFLAGS)
AC_SUBST(LIBCURL_LIBS)
AC_DEFINE([HAVE_LIBCURL], [], [libCURL is found and operational])

dnl check for libuuid
PKG_CHECK_MODULES(UUID, [uuid >= 1.0])
AC_SUBST(UUID_CFLAGS)
AC_SUBST(UUID_LIBS)
AC_DEFINE([HAVE_UUID], [], [libUUID is found and operational])

dnl FastCGI library check
dnl host_os is set by AC_CANONICAL_HOST
case $host_os in
    linux*) FCGIPATH=/usr ;;
    *bsd*)  FCGIPATH=/usr/local ; LDFLAGS="-L/usr/local/lib $LDFLAGS" ;;
    *)      AC_MSG_ERROR([Your platform is not currently supported]) ;;
esac
AC_ARG_WITH(fcgilib, [ --with-fcgilib=[libfcgi base path]],
            FCGIBASE=$withval, FCGIBASE=$FCGIPATH)
if test -f ${FCGIBASE}/include/fcgiapp.h
then
    FCGI_CXXFLAGS="-I$FCGIBASE/include"
    AC_SUBST(FCGI_CXXFLAGS)
else
    AC_MSG_ERROR([fcgiapp.h is not found or unusable])
fi
AC_SEARCH_LIBS(FCGX_OpenSocket, fcgi,
               FCGI_LDFLAGS="-lfcgi",
               AC_MSG_ERROR([FastCGI library is not found!]))
AC_SUBST(FCGI_LDFLAGS)
AC_DEFINE([HAVE_LIBFCGI], [], [libfcgi is found and operational])

dnl PostgreSQL libpqxx
PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 4.0])
AC_SUBST(LIBPQXX_CFLAGS)
AC_SUBST(LIBPQXX_LIBS)
AC_DEFINE([HAVE_LIBPQXX], [], [PostgeSQL C++ library is found and operational])

dnl check for bsd library on linux platform
if test "x$host_os" == xlinux-gnu
then
AC_SEARCH_LIBS(strlcat, bsd, BSD_LIB="-lbsd", AC_MSG_ERROR([libbsd is not found!]))
AC_DEFINE([HAVE_LIBBSD], [], [libbsd is found and operational])
fi
AC_SUBST(BSD_LIB)

dnl Check if we are compiling with  clang on linux platform. if so than add -lstdc++
dnl if test "x$host_os" == xlinux-gnu ; then
STDCXX_LIB=-lstdc++
AC_SUBST(STDCXX_LIB)
dnl fi

dnl Check for doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN],
           [AC_CONFIG_FILES([doc/Doxyfile])
            AC_DEFINE([HAVE_DOXYGEN], [], [Doxigen is found])],
           [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])])

dnl set Doxygen language
AC_ARG_WITH(doxylang,
            [  --with-doxylang=[language to use in documentation.] 
                      [See doc/Doxyfile.in for available languages]
                      [default English]], 
            DOXYLANG=${withval}, DOXYLANG="English")
AC_SUBST(DOXYLANG)

dnl set version control revision number and commit hash
if test -f vcrevision ; then
   VCREVISION=`cat vcrevision`
else
   VCREVISION=`git rev-list HEAD | wc -l`
fi
if test -f vccommit ; then
   VCCOMMIT=`cat vccommit`
else
   VCCOMMIT=`git rev-list HEAD | head -1`
fi
AC_SUBST(VCREVISION)
AC_SUBST(VCCOMMIT)

osname=`uname`
AC_CONFIG_FILES([
Makefile
src/Makefile
src/templates.cpp
include/Makefile
conf/Makefile
conf/appserver.$osname
test/Makefile
script/Makefile
doc/Makefile])

AC_OUTPUT


