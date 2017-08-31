#! /bin/bash

cd earthenterprise/earth_enterprise && \
scons -j8 release=1 build
