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


"""Module to check Fusion databases pushed to server for consistency.

The check is based on geindexcheck result.
"""

import getopt
import os
import re
import subprocess
import sys

BIN_DIR = "/opt/google/bin"
STREAM_SPACE_CONF_PATH = "/opt/google/gehttpd/conf.d/stream_space"


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
    results of the linux shell command running.
  """
  print "Executing: %s" % os_cmd

  try:
    p = subprocess.Popen(
        os_cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT if err2out else subprocess.PIPE)
    result, error = p.communicate()

    if (not err2out) and error:
      print "ERROR: %s" % error
      Die("Unable to execute %s" % os_cmd)

    return result
  except Exception, e:
    Die("FAILED: %s" % str(e))


def MatchPattern(file_path, pattern):
  """Looks for matching specified pattern in the file line by line.

  Args:
    file_path: file path.
    pattern: pattern for matching.
  Returns:
    the match or None.
  """
  try:
    with open(file_path, "r") as f:
      prog = re.compile(pattern)
      for line in f:
        match = prog.match(line)
        if match:
          return match
  except IOError:
    pass
  except Exception:
    pass

  return None


def DiscoverPublishStreamSpace():
  """Greps the installed stream_space config for the stream_space path."""

  stream_space_conf = STREAM_SPACE_CONF_PATH
  match = MatchPattern(stream_space_conf,
                       r"^Alias\s+/stream_space\s+(.*)")
  if not match:
    Die("No publish stream_space path found in %s" % stream_space_conf)

  stream_space_root = match.groups()[0]
  if not os.path.exists(stream_space_root):
    Die("Invalid publish stream_space path %s found in %s" % (
        stream_space_root, stream_space_conf))

  return stream_space_root


def FindAssets(assetroot, top_asset_pattern, report_abs_path=False):
  """Traverse the assetroot to find the names of assets.

  Args:
    assetroot: [publish] assets root path.
    top_asset_pattern: top asset pattern for regular expression.
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


def FindGeAndMapDbAssets(assetroot):
  """Traverse the assetroot to find the paths of published database assets.

  Args:
    assetroot: the [publish] assets root path.
  Returns:
    a list of asset paths.
  """
  top_asset_pattern = r"(gedb|mapdb)$"
  return FindAssets(assetroot, top_asset_pattern)


def RunGeindexcheck(db_asset, assetroot):
  """Runs geindexcheck for specified db asset.

  Args:
    db_asset: a database asset path.
    assetroot: the [publish] assets root path.
  Returns:
    whether the run is successful.
  """
  db_asset_path = os.path.normpath(
      os.path.join(assetroot, db_asset) if assetroot else db_asset)

  geindexcheck_path = os.path.join(BIN_DIR, "geindexcheck")
  result = ExecuteCmd(
      "%s --mode all --database %s" % (geindexcheck_path, db_asset_path),
      err2out=True)

  if "Fusion Fatal:" in result:
    print "FAILED"
    return False
  else:
    print "SUCCESS"
    return True


def RunGeindexcheckForDbs(db_assets, assetroot=None):
  """Runs geindexcheck for list of db assets.

  Args:
    db_assets: a list of database asset names/paths.
    assetroot: the [publish] assets root path.
  Returns:
    list of assets which geindexcheck has failed for.
  """
  assert isinstance(db_assets, list)
  db_assets_failed = []
  for asset in db_assets:
    if not RunGeindexcheck(asset, assetroot):
      db_assets_failed.append(asset)

  return db_assets_failed


def PrintDbAssets(db_assets):
  """Prints database assets.

  Args:
    db_assets: a list of database asset names/paths.
  """
  assert isinstance(db_assets, list)
  first_elem_of_path_pattern = r"\/?([^\/]+)"
  prog = re.compile(first_elem_of_path_pattern)

  for asset in db_assets:
    match_result = prog.match(asset)
    if not match_result:
      print ("WARNING: Couldn't get Fusion hostname"
             " from the asset path: %s" % asset)
      continue

    fusion_host = match_result.groups()[0]
    db_path = asset[len(fusion_host) : ]
    print "%s --fusion_host %s" % (db_path, fusion_host)


def ExecuteCheck(db_assets, assetroot):
  """Executes check for db assets.

  Args:
    db_assets: a list of db asset names/paths.
    assetroot: the [publish] assets root path.
  """
  print ""
  db_assets_failed = RunGeindexcheckForDbs(db_assets, assetroot)

  total_db_assets = len(db_assets)
  num_succeeded = total_db_assets - len(db_assets_failed)
  print ""
  print "%d/%d Fusion databases geindexcheck SUCCEEDED." % (
      num_succeeded, total_db_assets)

  if db_assets_failed:
    print ""
    print "Fusion databases geindexcheck FAILED:"
    PrintDbAssets(db_assets_failed)

    print ""
    print "To fix, a database needs to be re-pushed on server:"
    print "1. Delete(un-register) the databases that needs to be repushed:"
    print "    <Remove> for database in GEE Server Admin or"
    print ("    geserveradmin --deletedb db_path [--fusion_host"
           " fusion_host_name]")
    print "2. Re-push the databases either with geserveradmin or Fusion UI:"
    print "    geserveradmin --adddb ..."
    print "    geserveradmin --pushdb ..."


def Usage(msg=None):
  """Prints usage message.

  Args:
    msg: additional info to print, e.g. error message.
  """
  if msg:
    print ""
    print msg

  print ""
  print ("Usage: gecheckpusheddbs.py [--help] [--dryrun]"
         " [publish_root_stream_space_path]")
  print ""
  print "Will run geindexcheck for all database assets in publish root."
  print "Note: it is recommended to run 'geserveradmin --garbagecollect' before"
  print "running the check."
  print ""
  print ("    --dryrun just reports a list of assets that would have been"
         " checked.")
  print ""
  print ("    publish_root_stream_space_path the publish root stream_space path"
         " to use.")
  print "    If unspecified, the current path is used."
  print "    Current: %s" % DiscoverPublishStreamSpace()


def main(dry_run, stream_space=None):
  if not stream_space:
    stream_space = DiscoverPublishStreamSpace()

  print ""
  print "Publish root stream_space:", stream_space

  db_assets = FindGeAndMapDbAssets(stream_space)

  if not db_assets:
    print "No database assets."
    exit(0)

  if dry_run:
    print ""
    print "Dry run..."
    print ""
    print "Database assets: "
    PrintDbAssets(db_assets)
    print ""
    print "Total number of Fusion databases:", len(db_assets)
    exit(0)

  ExecuteCheck(db_assets, stream_space)


if __name__ == "__main__":
  opt_dryrun = 0
  stream_space_path = None

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
    stream_space_path = args[0]

  main(opt_dryrun, stream_space_path)
