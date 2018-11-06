#!/bin/bash

ASSETROOT=/gevol/assets

sudo service gefusion stop
sudo rm -rf ${ASSETROOT}/gauge_tests
sudo service gefusion start
