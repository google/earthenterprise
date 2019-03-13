#!/bin/bash

# Arguments: balaValidator.sh Test_No <Bool_Presence_of_File_Is_SUCCESS> <FILE_LOCATION> DESCRIPTION
# Bool_Presence_of_File_Is_SUCCESS = 1, if the success is interpreted as the presence of khasset.xml file.

if [ "$1" == "" ]
then
   echo "./$0 Test_No <Bool_Presence_of_File_Is_SUCCESS> <FILE_LOCATION> DESCRIPTION"
   exit 1;
fi

if [ "$2" == "pass" ]
then
   fileValidationTestArgumentForScript=""
else
   fileValidationTestArgumentForScript="!"
fi

if [ $fileValidationTestArgumentForScript -f  $3.kiasset/khasset.xml ]
then
   echo "$1: $4: PASS"
else
   echo "$1: $4: FAIL"
fi

