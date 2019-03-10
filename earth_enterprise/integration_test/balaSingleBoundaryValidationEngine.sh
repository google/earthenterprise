#!/bin/bash


################### SETUP ##################

commandPath=/opt/google/bin/
commandToTest=genewimageryresource

outputROOT=/gevol/assets


outputDirectoryPrefix=genewimageryresourceTestDirection
DATE=`date +%Y.%m.%d`
TIME=`date +%H.%M.%S`

# At present, I do not know the repercussions of having same directory name for multiple runs, and the implications as well.  So, creating unique names.
# FYI: Unique names will be generated through appending $LINENO as well.

outputDirectoryForTest=$outputDirectoryPrefix.$DATE.$TIME

testOptionName=error
if [ "$1" = "north_boundary" ]
then
   testOptionName=north_boundary
fi
if [ "$1" = "south_boundary" ]
then
   testOptionName=south_boundary
fi
if [ "$1" = "east_boundary" ]
then
   testOptionName=east_boundary
fi
if [ "$1" = "west_boundary" ]
then
   testOptionName=west_boundary
fi

if [ "$testOptionName" = "error" ]
then
   echo "ERROR in arguments for calling test script"
   return
fi



################### EXECUTING ALL TESTS ##################

echo "Executing Test case: for $testOptionName"

# Simple test case
testDescription="Simple Successful Case: just Integer"
testDir=$outputDirectoryForTest.$LINENO 

$commandPath/$commandToTest --$testOptionName 0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"


# Simple test case
# FYI: Preferred testing: Ensure generated files are EXACTLY the same as the PLUS zero case
testDescription="Simple Successful Case: just Negative Integer"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName -0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"






# Simple test case
# FYI: Preferred testing: Ensure generated files are EXACTLY the same as the PLUS zero case
testDescription="Simple Successful Case: simple double"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName 0.0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"




# Simple test case
# FYI: Preferred testing: Ensure generated files are EXACTLY the same as the PLUS zero case
testDescription="Simple Successful Case: just negative double"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName -0.0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"




# Boundary Cases
testDescription="Boundary case: Positive MAX boundary"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName 90.0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ ! -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription : For +90 degrees?"
fi
echo "---------------------------------------"




# Boundary Cases
testDescription="Boundary case: Negative MAX boundary (Or can be termed as: MIN)"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName -90.0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ ! -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription"
fi
echo "---------------------------------------"




# Boundary Cases
testDescription="Boundary case: BEYOND MAX boundary"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName 1000.0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ ! -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription : For +1000 degrees?"
fi
echo "---------------------------------------"



# Boundary Cases
testDescription="Boundary case: BEYOND MIN boundary"
testDir=$outputDirectoryForTest.$LINENO 
$commandPath/$commandToTest --$testOptionName -1000.0 -o $testDir /opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif

if [ ! -f $outputROOT/$testDir.kiasset/khasset.xml ]
then
   echo SUCCESSFUL: $testDescription
else
   echo "**** FAILURE: $testDescription : For -1000 degrees"
fi
echo "---------------------------------------"




################### TEAR DOWN ##################

#FYI: The files are presently created as a different user.  So, we need to invoke setuid before invoking rm -f
# rm -fR $outputROOT/$outputDirectoryForTest.*
