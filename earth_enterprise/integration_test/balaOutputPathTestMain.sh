#!/bin/bash

# For code reuse: HAVE to put this in a PREFIXED directory.
# Reason: To invoke the correct script, we either need exact directory location, or relative path.
# We can discuss this later... I might be wrong.  Please excuse this 3 line statement among multiple files.

# Change the directory to the location where the main script is located.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR





################### SETUP ##################

outputDirectoryPrefix=genewimageryresourceTestOutput
DATE=`date +%Y.%m.%d`
TIME=`date +%H.%M.%S`

# At present, I do not know the repercussions of having same directory name for multiple runs, and the implications as well.  So, creating unique names.
# FYI: Unique names will be generated through appending $LINENO as well.

outputDirectoryForTest=$outputDirectoryPrefix.$DATE.$TIME
outputROOT=/gevol/assets


################### EXECUTING ALL TESTS ##################

# For Randy: Ideally the test case numbers should be a number and incremented, rather than hard assignment


# Using the testDir in two places in the following command, one for input and another for output.
# So, better to have it as a variable here, so that there are no problems of copy paste later
testDir=$outputDirectoryForTest.$LINENO
./balaOutputPathTestDriver.sh 1 "Simple Successful Case" $testDir $outputROOT/$testDir "pass"

# Have asset name containing /
testDir=$outputDirectoryForTest.$LINENO/anotherDirectory
./balaOutputPathTestDriver.sh 2 "Multi Level Directory Output" $testDir $outputROOT/$testDir "pass"

# Have asset name starting with /
testDir=../../../../../../../../../../../../$outputROOT/$outputDirectoryForTest.$LINENO
./balaOutputPathTestDriver.sh 3 "FORCE MULTIPLE UPPER DIRECTORIES TO REACH ROOT DIRECTORY for output Directory" $testDir $outputROOT/$testDir "pass"

# Have asset name starting with /
testDescription="FLAW: SAME Directory as previous case, but instead with directly starting with / for output path"
testDir=$outputROOT/$outputDirectoryForTest.$LINENO
./balaOutputPathTestDriver.sh 4 "FLAW: SAME Directory as previous case, but instead with directly starting with / for output path" $testDir $outputROOT/$testDir "pass"

# Have asset with permission issue directory: Eg /etc
# testDescription="Test with cases for directory where we do not have permission to write"
echo SKIPPING TEST: $testDescription : Reason: Everything top level directory is already with ROOT permissions, which is makes this testcase not possible to write.
# TBD

# Have asset name with Known Special Characters
# The following characters are not allowed: 
#    & % ' \ " * = + ~ ` ? < > : ; and the space character.
testDir=$outputDirectoryForTest"&"$LINENO
./balaOutputPathTestDriver.sh 5 "NEGATIVE TEST: Asset name containing KNOWN special characters" $testDir $outputROOT/$testDir "fail"

# Have asset name with Unspecified Special Characters
# FYI: The failure comment notated does not contain these SPECIAL characters, but it still fails.
# Error Statement printed out: 
# The following characters are not allowed: 
#    & % ' \ " * = + ~ ` ? < > : ; and the space character.
testDir=$outputDirectoryForTest"&"$LINENO
./balaOutputPathTestDriver.sh 6 "Asset name containing Unspecified special characters" $testDir."\tHASTAB" $outputROOT/$testDir "pass"


# Simple test case
testDir=$outputDirectoryForTest"&"$LINENO
./balaOutputPathTestDriver.sh 7 "NEGATIVE: INVALID / Empty output directory" "" $outputROOT/$testDir "fail"

exit

