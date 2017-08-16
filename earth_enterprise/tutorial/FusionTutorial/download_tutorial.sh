#!/bin/bash

TMPFILE="/tmp/FusionTutorial-Full.tar.gz"
SCRIPTDIR=`dirname "$0"`

if [ "$EUID" != "0" ] ; then
    echo `basename $0` "must be run as root"
    exit 1;
fi

if [ ! -d "$SCRIPTDIR/Imagery" ]; then
	wget "http://data.opengee.org/FusionTutorial-Full.tar.gz" -O "$TMPFILE"
	tar -xvzf "$TMPFILE" -C "$SCRIPTDIR"
	rm -f "$TMPFILE"
fi
