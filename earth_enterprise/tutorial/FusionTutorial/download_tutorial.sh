#!/bin/bash

TMPFILE="/tmp/FusionTutorial-Full.tar.gz"
SCRIPTDIR=`dirname "$0"`
SELFNAME=`basename "$0"`

if [ $(id -u) != 0 ] ; then
    echo "$SELFNAME must be run as root"
    exit 1;
fi

if [ ! -d "$SCRIPTDIR/Imagery" ]; then
    wget "https://opengee-dev.s3.amazonaws.com/FusionTutorial-Full.tar.gz" -O "$TMPFILE"
    tar -xvzf "$TMPFILE" -C "$SCRIPTDIR"
    rm -f "$TMPFILE"
    chown -R gefusionuser:gegroup "$SCRIPTDIR"
fi
