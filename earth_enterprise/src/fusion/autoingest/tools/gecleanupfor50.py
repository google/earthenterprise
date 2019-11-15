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


"""Clean all asset versions that are not compatibile with Fusion 5.0.

Flat Databases (assets that end with .kmdatabase):
Fusion 5.0 no longer supports flat projects maps

Databases with Imagery or Terrain
Fusion 5.0 has a different output format for imagery and terrain projects.
So older databases have to be clean so they can't be served

Imagery and Terrain projects
Fusion 5.0 has a different output format for imagery and terrain projects.
So older projects have to be cleaned so pieces with the old format cannot be
reused with the new builds.
"""


import getopt
import os
import re
import subprocess
import sys

opt_dryrun = 0
bindir = '/opt/google/bin'


def main():
  assetroot = DiscoverAssetRoot()
  top_assets = FindTopAssets(assetroot)

  # clean all FlatDatabases
  for asset in top_assets.get('.kmdatabase', []):
    for version in GetNonCleanedVersions(asset):
      MyExec([os.path.join(bindir, 'geclean'), version])

  # check if Normal DBs have imagery or terrain
  for asset in top_assets.get('.kdatabase', []):
    for version in GetNonCleanedVersions(asset):
      # see if this version has either an imagery or terrain project
      if GrepFile(VersionConfigFile(assetroot, version),
                  r'<input>.+\.k[it]project.+</input>'):
        MyExec([os.path.join(bindir, 'geclean'), version])

  # Now that the databases are clean, we can clean the raster projects
  for asset_type in ('.kiproject', '.ktproject'):
    for asset in top_assets.get(asset_type, []):
      for version in GetNonCleanedVersions(asset):
        MyExec([os.path.join(bindir, 'geclean'), version])


def Die(msg):
  print >> sys.stderr, msg
  exit(1)


def DiscoverAssetRoot():
  """Grep the installed systemrc for the assetroot."""
  rcpath = '/opt/google/etc/systemrc'
  systemrc = open(rcpath)
  contents = systemrc.read()
  systemrc.close()
  match = re.search('<assetroot>(.+)</assetroot>', contents)
  if not match:
    Die('No assetroot found in %s' % rcpath)
    exit(1)
  assetroot = match.groups()[0]
  if not os.path.exists(assetroot):
    Die('Invalid assetroot %s found in %s' % (assetroot, rcpath))
  return assetroot


def GrepFile(path, pattern):
  file_handle = open(path)
  contents = file_handle.read()
  file_handle.close()
  # return a bool
  return not not re.search(pattern, contents)


def FindTopAssets(assetroot):
  """Traverse the assetroot to find the names of all top-level assets."""

  top_asset_regex = re.compile('((\.k[vitm]'
                               '(source|asset|project|layer|masset|mproject))|'
                               '\.kdatabase|\.kmdatabase|\.kmmdatabase)$')
  top_assets = {}
  for root, dirs, unused_ in os.walk(assetroot, topdown=True):
    asset_path = root[len(assetroot)+1:]  # path under assetroot
    descend_dirs = []
    for dname in dirs:
      if re.search(top_asset_regex, dname):
        relpath = os.path.join(asset_path, dname)
        asset_type = os.path.splitext(dname)[1]
        if asset_type in top_assets:
          top_assets[asset_type].append(relpath)
        else:
          top_assets[asset_type] = [relpath]
      else:
        descend_dirs.append(dname)
    del dirs[:]
    dirs.extend(descend_dirs)
  return top_assets


def GetNonCleanedVersions(asset):
  """Find all versions of the given asset that haven't already been cleaned."""
  non_clean = []
  # query list of versions
  output = subprocess.Popen(
      [os.path.join(bindir, 'gequery'), '--versions', asset],
      stdout=subprocess.PIPE).communicate()[0].strip()
  if output:
    lines = output.split('\n')
    for line in lines:
      match = re.search('^(.*version=\d*):\s+(\w+)$', line)
      if match:
        ver, state = match.groups()
        # if not already clean, clean it
        if state != 'Cleaned':
          non_clean.append(ver)
  return non_clean


def VersionConfigFile(assetroot, version):
  match = re.search('^(.*)\?version=(\d+)$', version)
  path, vernum = match.groups()
  return os.path.join(assetroot, path,
                      'ver%03d' % (int(vernum)),
                      'khassetver.xml')


def MyExec(cmd):
  if not opt_dryrun:
    print ' '.join(cmd)
    subprocess.check_call(cmd)
  else:
    print 'dry run:', ' '.join(cmd)


def Usage(msg):
  if msg: print msg
  print 'usage: gecleanupfor50.py [--help] [--dryrun]'
  print '   Will clean up old versions of assets, projects and databases'
  print '   that are not compatible with Fusion 5.0'
  print ''
  print '   --dryrun just reports what would have been done'


if __name__ == '__main__':
  opts, args = getopt.getopt(sys.argv[1:], 'hd', ['help', 'dryrun'])
  for opt, var in opts:
    if opt in ('-h', '--help'):
      Usage(' ')
      exit(0)
    elif opt in ('-d', '--dryrun'):
      opt_dryrun = 1

  if args:
    if args:
      Usage('No arguments allowed')
      exit(0)

  main()
