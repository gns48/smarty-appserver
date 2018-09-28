#!/bin/sh

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

# revision number (total number of commits)
VCREVISION=`ssh ${gituhost} "cd ${gitrpath} && git rev-list HEAD | wc -l" | sed 's/ //g'`

# last commit hash
VCCOMMIT=`ssh ${gituhost} "cd ${gitrpath} && git rev-list HEAD | head -1"`

# Snapshot date (YYYY-MM-DD)
SNAPDATE=`date +%F`

# tar directory prefix and file name
tarprefix="$rname-$VCREVISION-$VCCOMMIT"
fmt="tar"
tarname="$ftpdir/$tarprefix.$fmt"
symlink="$ftpdir/$rname-$SNAPDATE.$fmt"
lastname="$ftpdir/${rname}-last.$fmt"

if ! test -f $tarname ; then
    # No such revision exists yet
    git archive --remote=${giturl} --format=${fmt} --prefix="${tarprefix}/" HEAD > $tarname
    
    # add revision number and commit hash
    curdir=`pwd`
    cd /tmp
    mkdir $tarprefix
    echo $VCREVISION > $tarprefix/vcrevision
    echo $VCCOMMIT   > $tarprefix/vccommit
    tar uf $tarname $tarprefix
    rm -rf $tarprefix
    cd $curdir

    gzip $tarname
    chgrp ftp ${tarname}.gz
    ln -sf ${tarname}.gz ${symlink}.gz
    ln -sf ${tarname}.gz ${lastname}.gz
fi
