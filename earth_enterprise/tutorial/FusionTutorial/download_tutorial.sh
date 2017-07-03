#!/bin/bash
if [ ! -d Imagery ]; then
	wget http://data.opengee.org/FusionTutorial-Full.tar.gz -O /tmp/FusionTutorial-Full.tar.gz
	tar -xvzf /tmp/FusionTutorial-Full.tar.gz -C .
	rm -f /tmp/FusionTutorial-Full.tar.gz
fi
