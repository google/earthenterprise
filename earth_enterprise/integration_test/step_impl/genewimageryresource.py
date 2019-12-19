from getgauge.python import step, before_scenario, Messages
import os, random, fnmatch, subprocess, datetime
from subprocess import Popen
import shutil

# Re-use some of the functionalities
import assets

# ------------------------
# Helpful Not-Step Methods
# ------------------------
def doesKhassetFileExist(sPathUntilDotKiasset):
   print "ls -l " + sPathUntilDotKiasset + ".kiasset/khasset.xml"
   return os.path.exists(sPathUntilDotKiasset + ".kiasset/khasset.xml" )

def executeCommand(sTestDir):
   sImageryRoot = "/opt/google/share/tutorials/fusion/";
   sImageryFile = "Imagery/bluemarble_4km.tif"
   
   # Download the imagery, if not available.
   sCommandToRun = "sudo bash " + sImageryRoot + "download_tutorial.sh >/dev/null 2>/dev/null"
   print sCommandToRun
   pHandle = subprocess.Popen(sCommandToRun, shell=True)
   assert (pHandle.wait() == 0)
   
   sCommandUnderTest = "/opt/google/bin/genewimageryresource"
   sCommand = sCommandUnderTest + " -o \"" + sTestDir + "\" " + sImageryRoot + sImageryFile + " 2>/dev/null"
   pHandle = subprocess.Popen(sCommand, shell=True)
   # Reason for the wait or poll is: if the process started but it returned FAILURE (example incorrect input, and the executable returned a non-zero response, the wait would have failed.
   assert ( (pHandle.wait() == 0) or (pHandle.poll() != None) )


# ------------------------
# Helpful ENV settings
# ------------------------
HOME = assets.get_env_value("HOME")
now = datetime.datetime.now()
outputDirectoryForTest = "genewimageryresourceTestOutput" + "." + now.strftime("%Y.%m.%d.%H.%M.%S")
outputROOT = "/gevol/assets"

# ---------------------------
# The Tests
# ------------------------
@step("perform GEE new imagery resource simple test")
def geenewimageryresourceSimpleTest() :
   testDir = outputDirectoryForTest + "SimpleTest"
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == True)

@step("perform GEE new imagery resource multi level directory test")
def geenewimageryresourceMultiLevelDirectoryTest() :
   testDir = outputDirectoryForTest + "MultiDir/dir1/dir2/dir3_prefix"
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == True)

@step("perform GEE new imagery resource root directory test")
def geenewimageryresourceDirStartsAtRoot() :
   testDir = "/gevol/assets/" + outputDirectoryForTest + "RootDir"
   executeCommand(testDir)
   assert (doesKhassetFileExist(testDir) == False)

@step("perform GEE new imagery resource having unacceptable characters test")
def geenewimageryresourceHavingUnacceptableCharacters() :
   testDir = outputDirectoryForTest + "whatever:-$else"
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == False)

@step("perform GEE new imagery resource having unspecified special characters test")
def geenewimageryresourceUnspecifiedSpecialCharacters() :
   testDir = outputDirectoryForTest + 'TEMP/VAL\tUE'
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == True), " Creating Files and Dirs with TAB char has been traiditionally allowed"

@step("perform GEE new imagery resource having empty directory test")
def geenewimageryresourceNegativeCaseEmptyDir() :
   testDir = ""
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == False)

