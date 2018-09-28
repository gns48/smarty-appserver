#!/bin/sh

if [ -z "$1" ] ; then
   echo "Usage: $0 <tagname>"
   exit 1
else
  tagname="$1"
fi

# repository name
rname=app_srv
# git user and host
gituhost="git@redmine.it64k.com"
# repo path on remote host
gitrpath="/usr/local/git/repositories/${rname}.git"
# full url
giturl="ssh://${gituhost}/${gitrpath}"
# get revisionn list connamd
gitrlist="git rev-list HEAD"
# local directory for snapshots
ftpdir="/var/ftp/appserver"
# tar archive format
fmt="tar"

tarfile="${ftpdir}/${tagname}.${fmt}"

TAGEXISTS=`ssh ${gituhost} "cd ${gitrpath} && git tag" | grep $tagname`
if test -z $TAGEXISTS ; then
   echo "No tag $tagname found!"
   exit 1
fi

if ! test -f ${tarfile}.gz ; then
    # revision number (total number of commits) for given tag
    VCREVISION=`ssh ${gituhost} "cd ${gitrpath} && git rev-list ${tagname} | wc -l" | sed 's/ //g'`
    # last commit hash for given tag
    VCCOMMIT=`ssh ${gituhost} "cd ${gitrpath} && git rev-list ${tagname} | head -1"`

    git archive --remote=${giturl} --format=${fmt} --prefix="${tagname}/" ${tagname} > ${tarfile}

    # add revision number and commit hash
    curdir=`pwd`
    cd /tmp
    mkdir $tagname
    echo $VCREVISION > $tagname/vcrevision
    echo $VCCOMMIT   > $tagname/vccommit
    tar uf $tarfile $tagname
    rm -rf $tagname
    cd $curdir

    gzip $tarfile
    chgrp ftp ${tarfile}.gz
else
    echo "file ${tarfile} already exists!"
    exit 1
fi
