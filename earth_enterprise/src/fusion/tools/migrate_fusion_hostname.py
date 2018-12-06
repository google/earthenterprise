#!/usr/bin/python
#
# TODO insert appropriate copyright info
#
# TODO insert license info

# 
# Tool to automate the necessary database changes after changing 
# the Fusion hostname, to avoid having to delete/re-push/re-publish 
# existing (already published) GEE databases
#
# Reference:
# This tool automates the steps found in "Fusion Issues":
# http://www.opengee.org/geedocs/answer/172780.html
# 
# The mode must be specified by the user:
#	- GEE Server only (no Fusion)
#	- Development System (both Fusion and GEE Server)
# Both have similar steps with minor differences.

import argparse
import subprocess
