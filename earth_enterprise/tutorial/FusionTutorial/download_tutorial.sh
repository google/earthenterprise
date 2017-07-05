#!/bin/bash

TMPFILE=/tmp/FusionTutorial-Full.tar.gz

if [ ! -d Imagery ]; then
	wget http://data.opengee.org/FusionTutorial-Full.tar.gz -O $TMPFILE
	tar -xvzf $TMPFILE -C .
	rm -f $TMPFILE
fi
