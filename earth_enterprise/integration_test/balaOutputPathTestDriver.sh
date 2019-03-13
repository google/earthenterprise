#!/bin/bash

# For code reuse: HAVE to put this in a PREFIXED directory.
# Reason: To invoke the correct script, we either need exact directory location, or relative path.
# We can discuss this later... I might be wrong.  Please excuse this 3 line statement among multiple files.

# Change the directory to the location where the main script is located.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR

testNumber="$1"
comment="$2"
testDir="$3"
resultDir="$4"
passFail="$5"

################### SETUP ##################


commandPath=/opt/google/bin/
commandToTest=genewimageryresource

# At present, I do not know the repercussions of having same directory name for multiple runs, and the implications as well.  So, creating unique names.
# FYI: Unique names will be generated through appending $LINENO as well.

$commandPath/$commandToTest -o "$testDir" /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif 2>/dev/null
./balaValidator.sh "$testNumber" "$passFail" "$resultDir" "$comment"

