#!/usr/bin/env python3.8
#
# Copyright 2021 The Open GEE Contributors
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
#


"""Update the Fusion version string.

It takes two flags --long long_version_name and --short short_version_name.
Then it opens all the files need to be updated and changes from current
long_version_name and short_version_name to these new values.

Note: The long version name needs to start with short version name.

Example Usage:
  ./update_fusion_version.py --long "3.2.0" --short "3.2"

"""


import datetime
import fileinput
import os
import sys


def FindUpdateCurrentVersion(fusion_version_file, long_version, short_version,
                             year):
  """Find and update long and short version names in the fusion_version_file.

  Args:
    fusion_version_file: Absolute filename for fusion_version.txt
    long_version: The new long_version to update to.
    short_version: The new short_version to update to.
    year: The current year to be used in copyright statement.

  Returns:
    A couple of string (in a list) representing current long and short
    respectively.

  Raises:
    AssertionError: Whenever anything fails.
  """
  stage = 0  # not yet reached long_version
  for line in fileinput.FileInput(fusion_version_file, inplace=1):
    if stage == 0:
      if not line.startswith('#'):
        stage = 1  # long_version reached
        old_long = line[:-1]
        print(long_version)
      else:
        # TODO: Create script to do this for all copyrights.
        if line.startswith('# Copyright'):
          print('# Copyright %d Google Inc. All Rights Reserved.' % year)
        else:
          print(line, end=' ')
    elif stage == 1:
      old_short = line[:-1]
      print(short_version)
      stage = 2  # short version reached
    else:
      raise AssertionError('Cannot comprehend line "%s" in %s' % (
          line, fusion_version_file))
  return (old_long, old_short)


def ChangeVersionInInstallerFiles(
    common_prefix, old_long, new_long, old_short, new_short):
  """For installer xml files change from old version to new version.

  Args:
    common_prefix: Common root for all files to change.
    old_long: Current long version string.
    new_long: New long version string.
    old_short: Current short version string.
    new_short: New short version string.

  Raises:
    AssertionError: Whenever anything fails.
  """
  installer_files = ('installer/config/GoogleEarthInstaller.iap_xml',
                     'installer/config/GoogleFusionInstaller.iap_xml',
                     'installer/config/GoogleFusionToolsInstaller.iap_xml')
  old_long_cdata = 'CDATA[%s]' % (old_long)
  old_short_cdata = 'CDATA[%s]' % (old_short)
  new_long_cdata = 'CDATA[%s]' % (new_long)
  new_short_cdata = 'CDATA[%s]' % (new_short)
  long_key = 'CDATA[$LONG_VERSION$]'
  short_key = 'CDATA[$SHORT_VERSION$]'
  for file_name in installer_files:
    file_name = '%s/%s' % (common_prefix, file_name)
    in_defered_mode = False
    defered_lines = []
    for line in fileinput.FileInput(file_name, inplace=1):
      if not in_defered_mode:
        if line.find(old_long_cdata) >= 0 or line.find(old_short_cdata) >= 0:
          in_defered_mode = True
          defered_lines.append(line)
        else:
          line = line.replace(old_long, new_long)
          print(line, end=' ')
      else:
        long_key_found = (line.find(long_key) >= 0)
        if long_key_found or (line.find(short_key) >= 0):
          if long_key_found:
            print(defered_lines[0].replace(old_long_cdata, new_long_cdata), end=' ')
          else:
            print(defered_lines[0].replace(old_short_cdata, new_short_cdata), end=' ')
          for index in range(1, len(defered_lines)):
            print(defered_lines[index], end=' ')
          print(line, end=' ')
          defered_lines = []
          in_defered_mode = False
        else:
          defered_lines.append(line)


def print_and_exit(argv):
  """
  Prints help info, and exits.
  """
  sys.stderr.write('Wrong Usage of the script %s \n\n' % argv[0])
  sys.stderr.write('Must provide both a short and long version that match!\n')
  sys.stderr.write('Short version must be in the form "x.x"\n')
  sys.stderr.write('Long version must be in the form "x.x.x"\n')
  sys.stderr.write('Example usage:\n')
  sys.stderr.write('    ./update_fusion_version.py --long "3.2.0" --short "3.2"\n')
  sys.exit(-1) 


def main(argv):
  short_ver = ''
  long_ver = ''
  if (('--short' not in argv) or ('--long' not in argv) or (len(argv) < 5)):
    print_and_exit(argv)
  elif ((argv[1] == '--short') and (argv[3] == '--long')):
    short_ver = argv[2]
    long_ver = argv[4]
  elif ((argv[1] == '--long') and (argv[3] == '--short')):
    short_ver = argv[4]
    long_ver = argv[2]
  else:
    print_and_exit(argv)
  
  if (short_ver.split('.') != long_ver.split('.')[:2]):
    print_and_exit(argv)
    
  script_path = os.path.abspath(argv[0])
  common_prefix = os.path.dirname(os.path.dirname(script_path))
  fusion_version_file = '%s/%s' % (common_prefix, 'src/fusion_version.txt')
  (old_long, old_short) = FindUpdateCurrentVersion(fusion_version_file,
                                                   long_ver, short_ver,
                                                   datetime.datetime.now().year)
  ChangeVersionInInstallerFiles(
      common_prefix, old_long, long_ver, old_short, short_ver)


if __name__ == '__main__':
  main(sys.argv)
