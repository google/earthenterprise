#!/bin/bash

TMPFILE="/tmp/FusionTutorial-Full.tar.gz"
SCRIPTDIR=`dirname "$0"`

if [ ! -d "$SCRIPTDIR/Imagery" ]; then
	wget "http://data.opengee.org/FusionTutorial-Full.tar.gz" -O "$TMPFILE"
	tar -xvzf "$TMPFILE" -C "$SCRIPTDIR"
	rm -f "$TMPFILE"
fi
