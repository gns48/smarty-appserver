#!/bin/sh

if [ -z "$1" ] ; then
    echo "Usage: $0 <tag>"
    exit 1
fi

tag=$1

if [ "`git tag | grep $tag`" = "$tag" ] ; then
    REV=`git rev-list $tag | wc -l | tr -d "[:blank:]"`
    echo $REV > vcrevision
    git rev-list $tag | head -1 > vccommit
    name="${tag}.${REV}"
    git archive --prefix=$name/ $tag  > ../$name.tar
    case `uname` in
	FreeBSD) tar rf ../$name.tar -s ",^,$name/," vcrevision vccommit ;;
	Linux) 	 tar rf ../$name.tar --transform "s,^,$name/," vcrevision vccommit ;;
    esac 
    xz ../$name.tar
    rm vcrevision vccommit
    exit 0
else
    echo "No $1 tag found!"
    exit 2
fi




















