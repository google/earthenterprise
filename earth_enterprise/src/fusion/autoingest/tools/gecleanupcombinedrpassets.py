#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Module cleans up CombinedRPAssetVersion config files in current assetroot.

It deletes PacketLevelAssetVersion <listener> items in CombinedRPAssetVersion
config files.
"""

import getopt
import getpass
import glob
import math
import os
import pwd
import re
import shutil
import subprocess
import sys
import tempfile


BIN_DIR = "/opt/google/bin"
SYSTEMRC_PATH = "/opt/google/etc/systemrc"
GEAPACHEUSER = "geapacheuser"


def Die(msg):
  print >> sys.stderr, msg
  exit(1)


def ExecuteCmd(os_cmd, err2out=False):
  """Execute shell command.

  If the shell command fails, exit(1).

  Args:
    os_cmd: (string) linux shell command to run.
    err2out: whether to send stderr to the same file handle as for stdout.
  Returns:
    (output_data, return_code) of the linux shell command running.
  """
  print "Executing: %s" % os_cmd

  try:
    p = subprocess.Popen(
        os_cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT if err2out else subprocess.PIPE)
    out_data, err_data = p.communicate()

    if (not err2out) and err_data:
      print "ERROR: %s" % err_data
      Die("Unable to execute %s" % os_cmd)

    return out_data, p.returncode
  except Exception, e:
    Die("FAILED: %s" % str(e))


def DiscoverAssetRoot():
  """Grep the installed systemrc for the assetroot."""
  rcpath = SYSTEMRC_PATH
  systemrc = open(rcpath)
  contents = systemrc.read()
  systemrc.close()
  match = re.search(r"<assetroot>(.+)</assetroot>", contents)
  if not match:
    Die("No assetroot found in %s" % rcpath)

  assetroot = match.groups()[0]
  if not os.path.exists(assetroot):
    Die("Invalid assetroot path %s found in %s" % (assetroot, rcpath))

  return assetroot


def FindAssets(assetroot, top_asset_pattern, report_abs_path=False):
  """Traverse the assetroot to find the names of assets.

  Args:
    assetroot: the assetroot path.
    top_asset_pattern: a top asset pattern for regular expression.
    report_abs_path: whether to report absolute path for assets.
  Returns:
    a list of asset names/paths.
  """
  top_asset_regex = re.compile(top_asset_pattern)
  top_assets = []
  for root, dirs, unused_ in os.walk(assetroot, topdown=True):
    # Use either absolute or relative path (path under assetroot).
    asset_path = root if report_abs_path else root[len(assetroot)+1:]

    for dname in dirs:
      if re.search(top_asset_regex, dname):
        full_path = os.path.join(asset_path, dname)
        top_assets.append(full_path)

  return top_assets


def FindCombinedRpAssets(assetroot):
  """Traverse the assetroot to find CobminedRP assets.

  Args:
    assetroot: the assetroot path.
  Returns:
    a list of CombinedRPAsset paths.
  """
  top_asset_pattern = r"(CombinedRP\.k[it]a)$"
  return FindAssets(assetroot, top_asset_pattern)


def GetAssetVersionConfigFiles(assetroot, asset_path):
  """Find all AssetVersion config files of the given asset in filesystem.

  Args:
    assetroot: the assetroot path.
    asset_path: the asset path.
  Returns:
    a list of AssetVersion config files paths.
  """
  # query list of versions
  pathname = "ver[0-9][0-9][0-9]/khassetver.xml"
  asset_abs_path = os.path.join(assetroot, asset_path)
  return glob.iglob("%s/%s" % (asset_abs_path, pathname))


def ListenersFilter(file_path):
  """File lines generator filtering packlevel's <listener> items.

  Reads specified version config file and filters packlevel's <listener> items.
  Args:
    file_path: assetversion config file path.
  Yields:
    All the lines except the <listener/> ones or Exception object.
  """
  pattern = r"<listener>.+packlevel\d+\.k[it]a.+</listener>$"

  try:
    with open(file_path, "r") as f:
      prog = re.compile(pattern)
      for line in f:
        match = prog.search(line)
        if not match:
          yield line
  except IOError as e:
    yield e
  except Exception as e:
    yield e


def DoClean(assetroot, assets):
  """Executes listeners cleaning in AssetVersion config files.

  Args:
    assetroot: the assetroot path.
    assets: a list of assets.
  """
  tmp_file = None
  total_cleaned = 0

  try:
    tmp_file = tempfile.NamedTemporaryFile(
        mode="w", prefix="tmp_gecleancombinedrp", delete=False)
    tmp_file.close()

    # Traverse all versions of CombinedRPAssets and clean listeners.
    print ""
    for asset in assets:
      for version_filepath in GetAssetVersionConfigFiles(assetroot, asset):
        print "Executing:", version_filepath
        # Note: backup could be expensive in terms of disk space usage.
        # shutil.copyfile(version_filepath, "%s.bak" % version_filepath)

        with open(tmp_file.name, mode="w") as tmp_fd:
          for item in ListenersFilter(version_filepath):
            if isinstance(item, Exception):
              raise item
            else:
              tmp_fd.write(item)
        # Rewrite source version config file with cleaned one.
        shutil.copyfile(tmp_file.name, version_filepath)
        total_cleaned += 1
        print "SUCCESS"

    print "%s CombinedRPAssetVersion config files CLEANED." % total_cleaned

    # Delete temp. file.
    if os.path.exists(tmp_file.name):
      os.unlink(tmp_file.name)
  except Exception as e:
    if tmp_file:
      # Close and delete temp. file
      tmp_file.close()
      if os.path.exists(tmp_file.name):
        os.unlink(tmp_file.name)

    Die("Error: %s" % str(e))


def GetPrintSize(size_bytes):
  """Convert size in bytes to readable representation in bytes/KB/MB/GB/TB/PB.

  Args:
    size_bytes: size in bytes
  Returns:
    a string representation of size either in bytes or KB/MB/GB/TB/PB.
  """
  size_names = ("bytes", "KB", "MB", "GB", "TB", "PB")
  if size_bytes == 0:
    return "%s %s" % (size_bytes, size_names[0])

  i = int(math.floor(math.log(size_bytes, 1024)))
  i = min(i, len(size_names) - 1)
  p = math.pow(1024, i)
  s = round(size_bytes/p, 3)
  return "%s %s" % (s, size_names[i])


def PrintAssetVersions(assetroot, assets):
  """Prints asset versions references.

  Args:
    assetroot: the assetroot path.
    assets: a list of assets.
  """
  total_config_files = 0
  total_config_files_size = 0
  for asset in assets:
    print ""
    print "Asset:", asset
    for version_config_filepath in GetAssetVersionConfigFiles(assetroot, asset):
      print "   ", version_config_filepath
      total_config_files_size += os.path.getsize(version_config_filepath)
      total_config_files += 1

  print ""
  print "Total AssetVersion config files (count/size): %s/%s" % (
      total_config_files, GetPrintSize(total_config_files_size))


def IsFusionRunning():
  """Checks whether Fusion daemons are running.

  Returns:
    whether Fusion daemons are running.
  """
  print ""
  unused_output, error_code = ExecuteCmd(
      "%s --checkrunning" % os.path.join(BIN_DIR, "gefdaemoncheck"))
  return error_code == 0


def SwitchEffectiveUserToThis(user_name):
  """Switches effective user to specified.

  Args:
    user_name: a user name.
  """
  print ""
  print "Switching effective user to:", user_name
  try:
    user_entry = pwd.getpwnam(user_name)
    user_id = user_entry.pw_uid
    group_id = user_entry.pw_gid
    os.setegid(group_id)
    os.seteuid(user_id)
  except KeyError as e:
    Die("Couldn't get uid/gid for user '%s'. Error: %s" % (user_name, str(e)))
  except Exception as e:
    Die("Couldn't switch effective user to '%s'. Error: %s" % (
        user_name, str(e)))


def Usage(msg=None):
  """Prints usage message.

  Args:
    msg: additional info to print, e.g. error message.
  """
  if msg:
    print ""
    print msg

  print ""
  print "Usage: gecleancombinedrpasset.py [--help] [--dryrun] [assetroot_path]"
  print ""
  print "Will clean up listeners in all CombinedRPAssetVersion config files."
  print ""
  print ("    --dryrun just reports a list of asset versions that would have"
         " been cleaned.")
  print ""
  print ("    assetroot_path the assetroot path to use. If uncpecified, the"
         " current assetroot")
  print "    path is used."
  assetroot = DiscoverAssetRoot()
  print "    Current assetroot:", assetroot


def main(dryrun, assetroot):
  if not assetroot:
    assetroot = DiscoverAssetRoot()

  print ""
  print "The assetroot path:", assetroot

  combinedrp_assets = FindCombinedRpAssets(assetroot)

  if dryrun:
    PrintAssetVersions(assetroot, combinedrp_assets)
    exit(0)

  if getpass.getuser() != "root":
    Die("You must run as root.")

  if IsFusionRunning():
    Die("Please stop fusion before proceeding: /etc/init.d/gefusion stop")

  geapacheuser = GEAPACHEUSER
  SwitchEffectiveUserToThis(geapacheuser)

  # Do listeners cleaning in AssetVersion config files.
  DoClean(assetroot, combinedrp_assets)


if __name__ == "__main__":
  opt_dryrun = 0
  assetroot_path = None

  try:
    opts, args = getopt.getopt(sys.argv[1:], "hd", ["help", "dryrun"])
  except getopt.GetoptError as e:
    Usage("Error: %s" % str(e))
    exit(1)

  for opt, var in opts:
    if opt in ("-h", "--help"):
      Usage()
      exit(0)
    elif opt in ("-d", "--dryrun"):
      opt_dryrun = 1

  if args:
    assetroot_path = args[0]

  main(opt_dryrun, assetroot_path)
