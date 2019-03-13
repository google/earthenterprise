import os, random, fnmatch, subprocess, datetime
from subprocess import Popen

# ------------------------
# Helpful Not-Step Methods
# ------------------------
def get_env_value(sKey):
    return os.getenv(sKey, "unset")

# ------------------------
# Helpful ENV settings
# ------------------------
HOME = get_env_value("HOME")
now = datetime.datetime.now()
outputDirectoryForTest = "genewimageryresourceTestOutput" + "." + now.strftime("%Y.%m.%d.%H.%M.%S")
outputROOT = "/gevol/assets"

# ---------------------------
# The Tests
# ------------------------
def genewimageryresourceSimpleTest() :
   print("\nDoing gennewimageryresource Simple Test\n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.1"
   sCommand = "./balaOutputPathTestDriver.sh 1 \"Simple Successful Case\" " + testDir + " " + outputROOT+"/"+testDir + " pass" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

def genewimageryresourceMultiLevelDirectoryTest() :
   print("\nDoing gennewimageryresource Multi Level Directory Test\n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.2/AdditionalDir"
   sCommand = "./balaOutputPathTestDriver.sh 2 \"Multi Level Directory\" " + testDir + " " + outputROOT+"/"+testDir + " pass" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

def genewimageryresourceDirectoryTraversal() :
   print("\nDoing gennewimageryresource Directory Traversal Test\n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.3"
   sCommand = "./balaOutputPathTestDriver.sh 3 \"Directory Traversal\" " + testDir + " " + outputROOT+"/"+testDir + " pass" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

def genewimageryresourceDirStartsAtRoot() :
   print("\nDoing gennewimageryresource RootDirectory Test\n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.4"
   sCommand = "./balaOutputPathTestDriver.sh 4 \"From Root Directory\" " + testDir + " " + outputROOT+"/"+testDir + " pass" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

def genewimageryresourceHavingUnacceptableCharacters() :
   print("\nDoing gennewimageryresource Having Unacceptable Characters Test\n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.5"
   sCommand = "./balaOutputPathTestDriver.sh 5 \"Negative: Having Unacceptable Characters\" " + testDir + " " + outputROOT+"/"+testDir + " fail" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

def genewimageryresourceUnspecifiedSpecialCharacters() :
   print("\nDoing gennewimageryresource UnspecifiedSpecialCharacters Test\n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.6"
   sCommand = "./balaOutputPathTestDriver.sh 6 \"UnspecifiedSpecialCharacters\" " + testDir + " " + outputROOT+"/"+testDir + " pass" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

def genewimageryresourceNegativeCaseEmptyDir() :
   print("\nDoing gennewimageryresource Negative case of Empty Directory \n")
   testDir = outputDirectoryForTest + "TBD.LINE.NUMBER.7"
   sCommand = "./balaOutputPathTestDriver.sh 7 \"Negative Test: Empty Directory\" " + testDir + " " + outputROOT+"/"+testDir + " fail" 
   print(sCommand)
   pHandle = subprocess.Popen(sCommand, shell=True)
   assert (pHandle.wait() == 0)

genewimageryresourceSimpleTest()
genewimageryresourceMultiLevelDirectoryTest()
genewimageryresourceDirectoryTraversal()
genewimageryresourceDirStartsAtRoot()
genewimageryresourceHavingUnacceptableCharacters()
genewimageryresourceUnspecifiedSpecialCharacters()
genewimageryresourceNegativeCaseEmptyDir()

