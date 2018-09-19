#!/usr/bin/env python
import os
#import sys
import argparse
import re
import git
import subprocess
from datetime import datetime

def GetLongVersion(backupFile, label=''):
  """Create a detailed version string based on the state of
     the software, as it exists in the repository."""
 
  if CheckGitAvailable():
    ret = GitGeneratedLongVersion()

  # Without git, must use the backup file to create a string.
  else:
    base = ReadBackupVersionFile(backupFile)
    date = datetime.utcnow().strftime("%Y%m%d%H%M")
    ret = '-'.join([base, date])

  # Append the label, if there is one.
  if len(label):
    ret = '.'.join([ret, label])

  return ret


def GetVersion(backupFile, label=''):
  """As getLongVersion(), but only return the leading *.*.* value."""

  raw = GetLongVersion(backupFile, label)
  final = raw.split("-")[0]

  return final


def GetRepository():
    """Get a reference to the Git Repository.
    Is there a cleaner option than searching from the current location?"""

    # The syntax is different between library versions (particularly,
    # those used by Centos 6 vs Centos 7).
    try:
        return git.Repo('.', search_parent_directories=True)
    except TypeError:
        return git.Repo('.')
 

def CheckGitAvailable():
    """Try the most basic of git commands, to see if there is
       currently any access to a repository."""
    
    try:
        repo = GetRepository()
    except git.exc.InvalidGitRepositoryError:
        return False
    
    return True


def CheckDirtyRepository():
    """Check to see if the repository is not in a cleanly committed state."""

    repo = GetRepository()
    str = repo.git.status("--porcelain")
    
    # Ignore version.txt for this purpose, as a build may modify the file
    # and lead to an erroneous interpretation on repeated consecutive builds.
    if (str == " M earth_enterprise/src/version.txt\n"):
        return False
    
    return (len(str) > 0)
    

def ReadBackupVersionFile(target):
  """There should be a file checked in with the latest version
     information available; if git isn't available to provide
     information, then use this file instead."""

  with open(target, 'r') as fp:
    line = fp.readline()

  return line

def GitGeneratedLongVersion():
    """Take the raw information parsed by git, and use it to
       generate an appropriate version string for GEE."""

    repo = GetRepository()
    raw = repo.git.describe('--tags', '--match', '[0-9]*\.[0-9]*\.[0-9]*\-*')
    raw = raw.rstrip()

    # Grab the datestamp.
    date = datetime.utcnow().strftime("%Y%m%d%H%M")

    # If this condition hits, then we are currently on a tagged commit.
    if (len(raw.split("-")) < 4):
        if CheckDirtyRepository():
            return '.'.join([raw, date])
        return raw

    # Tear apart the information in the version string.
    components = ParseRawVersionString(raw)
  
    # Determine how to update, since we are *not* on tagged commit.
    if components['isFinal']:
        components['patch'] = 0
        components['patchType'] = "alpha"
        components['revision'] = components['revision'] + 1
    else:
        components['patch'] = components['patch'] + 1
    
    # Rebuild.
    base = '.'.join([str(components[x]) for x in ("major", "minor", "revision")])
    patch = '.'.join([str(components["patch"]), components["patchType"], date])
    if not CheckDirtyRepository():
        patch = '.'.join([patch, components['hash']])
    
    return '-'.join([base, patch])


def ParseRawVersionString(raw):
    """Break apart a raw version string into its various components,
    and return those entries via a dictionary."""

    components = { }    
    rawComponents = raw.split("-")
    
    base = rawComponents[0]
    patchRaw = rawComponents[1]
    components['numCommits'] = rawComponents[2]
    components['hash'] = rawComponents[3]
    components['isFinal'] = ((patchRaw[-5:] == "final") or
                             (patchRaw[-7:] == "release"))
  
    baseComponents = base.split(".")
    components['major'] = int(baseComponents[0])
    components['minor'] = int(baseComponents[1])
    components['revision'] = int(baseComponents[2])
  
    patchComponents = patchRaw.split(".")
    components['patch'] = int(patchComponents[0])
    if (len(patchComponents) < 2):
        components['patchType'] = "alpha"
    else:
        components['patchType'] = patchComponents[1]
        
    return components
  
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--long", action="store_true", help="Output long format of version string")
    args = parser.parse_args()

    # default parameter to GetVersion functions
    self_path, _ = os.path.split(os.path.realpath(__file__))
    version_file = os.path.join(self_path, '../version.txt')

    version = ""
    if args.long:
        version = GetLongVersion(version_file)
    else:
        version = GetVersion(version_file)

    print version

if __name__ == "__main__":
    main()