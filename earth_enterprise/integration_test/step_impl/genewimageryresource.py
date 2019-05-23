from getgauge.python import step, before_scenario, Messages
import os, random, fnmatch, subprocess, datetime
from subprocess import Popen
import shutil

# ------------------------
# Helpful Not-Step Methods
# ------------------------
def get_env_value(sKey):
   return os.getenv(sKey, "unset")

def doesKhassetFileExist(sPathUntilDotKiasset):
   return os.path.exists(sPathUntilDotKiasset + ".kiasset/khasset.xml" )

def executeCommand(sTestDir):
   sImageryRoot = "/opt/google/share/tutorials/fusion/";
   sImageryFile = "Imagery/bluemarble_4km.tif"
   
   # What if the Imagery wasn't downloaded
   sCommandToRun = "sudo " + sImageryRoot + "download_tutorial.sh >/dev/null 2>/dev/null"
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
HOME = get_env_value("HOME")
now = datetime.datetime.now()
outputDirectoryForTest = "genewimageryresourceTestOutput" + "." + now.strftime("%Y.%m.%d.%H.%M.%S")
outputROOT = "/gevol/assets"

sCommandScript = "./step_impl/balaOutputPathTestDriver.sh "

# ---------------------------
# The Tests
# ------------------------
@step("perform ge new imagery resource simple test")
def genewimageryresourceSimpleTest() :
   testDir = outputDirectoryForTest + "SimpleTest"
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == True)

@step("perform ge new imagery resource multi level directory test")
def genewimageryresourceMultiLevelDirectoryTest() :
   testDir = outputDirectoryForTest + "MultiDir/dir1/dir2/dir3_prefix"
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == True)

@step("perform ge new imagery resource root directory test")
def genewimageryresourceDirStartsAtRoot() :
   testDir = "/gevol/assets/" + outputDirectoryForTest + "RootDir"
   executeCommand(testDir)
   assert (doesKhassetFileExist(testDir) == False)

@step("perform ge new imagery resource having unacceptable characters test")
def genewimageryresourceHavingUnacceptableCharacters() :
   testDir = outputDirectoryForTest + "whatever:-$else"
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == False)

@step("perform ge new imagery resource having unspecified special characters test")
def genewimageryresourceUnspecifiedSpecialCharacters() :
   testDir = outputDirectoryForTest + 'TEMP/VAL\tUE'
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == True), " Creating Files and Dirs with TAB char has been traiditionally allowed"

@step("perform ge new imagery resource having empty directory test")
def genewimageryresourceNegativeCaseEmptyDir() :
   testDir = ""
   executeCommand(testDir)
   assert (doesKhassetFileExist(outputROOT+"/"+testDir) == False)

