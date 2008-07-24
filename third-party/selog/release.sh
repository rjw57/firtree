#!/bin/sh -e
#
# Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
# at the University of Cambridge Computing Service.
# You may do anything with this, at your own risk.
#
# $Cambridge: users/fanf2/selog/release.sh,v 1.5 2008/04/10 10:22:39 fanf2 Exp $

date=$1
ver=$2

repo=`cat CVS/Repository`
root=`cat CVS/Root`

mkdir -p Web
cd Web

selog=selog-$ver
cvs -d $root export -D "$date" -d $selog $repo
cd $selog
make docs
rm -rf Web
cd ..
echo tar cfz $selog.tar.gz $selog
tar cfz $selog.tar.gz $selog

exit
