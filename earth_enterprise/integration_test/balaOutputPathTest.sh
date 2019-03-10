#!/bin/bash



################### SETUP ##################

commandPath=/opt/google/bin/
commandToTest=genewimageryresource

outputROOT=/gevol/assets


outputDirectoryPrefix=genewimageryresourceTestOutput
DATE=`date +%Y.%m.%d`
TIME=`date +%H.%M.%S`

# At present, I do not know the repercussions of having same directory name for multiple runs, and the implications as well.  So, creating unique names.
# FYI: Unique names will be generated through appending $LINENO as well.

outputDirectoryForTest=$outputDirectoryPrefix.$DATE.$TIME



################### EXECUTING ALL TESTS ##################


# Simple test case
testDescription="Simple Successful Case"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"





# Have asset name containing /
testDescription="Multi Level Directory Output"
testDir=$outputDirectoryForTest.$LINENO/anotherDirectory 
$commandPath/$commandToTest -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"







# Have asset name starting with /
testDescription="FORCE MULTIPLE UPPER DIRECTORIES TO REACH ROOT DIRECTORY for output Directory"
testDir=../../../../../../../../../../../../$outputROOT/$outputDirectoryForTest.$LINENO
$commandPath/$commandToTest -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"






# Have asset name starting with /
testDescription="FLAW: SAME Directory as previous case, but instead with directly starting with / for output path"
testDir=$outputROOT/$outputDirectoryForTest.$LINENO
$commandPath/$commandToTest -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"





# Have asset with permission issue directory: Eg /etc
testDescription="Test with cases for directory where we do not have permission to write"
echo SKIPPING TEST: $testDescription : Reason: Everything top level directory is already with ROOT permissions, which is makes this testcase not possible to write.
# FYI: There is an alternative way: Create a directory from different user
# Then test writing through to that directory.
# It needs a bit more time and elaborate testing.
echo "---------------------------------------"


# Have asset name with Known Special Characters
# The following characters are not allowed: 
#    & % ' \ " * = + ~ ` ? < > : ; and the space character.
testDescription="Asset name containing KNOWN special characters"
testDir=$outputDirectoryForTest"&"$LINENO
# FYI: EXPECTING FAILURE.
# So, Redirecting output AND ERROR streams to /dev/null
$commandPath/$commandToTest -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif 2>/dev/null

if [ ! -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"



# Have asset name with Unspecified Special Characters
# FYI: The failure comment notated does not contain these SPECIAL characters, but it still fails.
# Error Statement printed out: 
# The following characters are not allowed: 
#    & % ' \ " * = + ~ ` ? < > : ; and the space character.

testDescription="Asset name containing Unspecified special characters"
originalTestDir=$outputDirectoryForTest.$LINENO
testDir=$originalTestDir."\tHASTAB"
$commandPath/$commandToTest -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif 2>/dev/null

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi

echo "---------------------------------------"


# Simple test case
testDescription="INVALID / Empty output directory"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest -o "" /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif 2>/dev/null

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"







################### TEAR DOWN ##################

#FYI: The files are presently created as a different user.  So, we need to invoke setuid before invoking rm -f
# rm -fR $outputROOT/$outputDirectoryForTest.*
