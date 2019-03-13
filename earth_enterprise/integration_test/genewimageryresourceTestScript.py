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
print("\nDoing gennewimageryresource Simple Test\n")

testDir = outputDirectoryForTest + "TBD.LINE.NUMBER"

sCommand = "./balaOutputPathTestDriver.sh " + " 1 " + "\"Simple Successful Case\" " + testDir + " " + outputROOT+"/"+testDir + " pass" 

pHandle = subprocess.Popen(sCommand, shell=True)
assert (pHandle.wait() != 0)




