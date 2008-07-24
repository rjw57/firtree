#!/bin/sh -e
#
# Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
# at the University of Cambridge Computing Service.
# You may do anything with this, at your own risk.
#
# $Cambridge: users/fanf2/selog/install.sh,v 1.2 2008/04/09 18:06:19 fanf2 Exp $

mode="$1"
dest="$2"
shift 2

run () {
	echo "$@"
	"$@"
}

mkdir -p "$dest"

for file in "$@"
do
	run cp -RP "$file" "$dest/$file"
	run chmod "$mode"  "$dest/$file"
done
