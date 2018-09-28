#!/bin/sh

# From configure.ac, assuming that version number is actual
# AC_INIT([appserver], [2.1], [gleb.semenov@gmail.com], , )
#         ^ --PKG-- ^  ^VER^
PKG=`grep AC_INIT configure.ac | cut -d\[ -f2 | cut -d\] -f1`
VER=`grep AC_INIT configure.ac | cut -d\[ -f3 | cut -d\] -f1`
tag="${PKG}-${VER}"

if [ "`git tag | grep $tag`" = "$tag" ] ; then
    REV=`git rev-list HEAD | wc -l | tr -d "[:blank:]"`
    echo $REV > vcrevision
    git rev-list HEAD | head -1 > vccommit
    name="${tag}.${REV}"
    git archive --prefix=$name/ HEAD > ../$name.tar
    case `uname` in
	FreeBSD) tar rf ../$name.tar -s ",^,$name/," vcrevision vccommit ;;
	Linux) 	 tar rf ../$name.tar --transform "s,^,$name/," vcrevision vccommit ;;
    esac 
    xz ../$name.tar
    rm vcrevision vccommit
    exit 0
else
    echo "No $tag tag found!"
    exit 2
fi
